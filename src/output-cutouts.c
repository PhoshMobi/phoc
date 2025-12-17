/*
 * Copyright (C) 2022-2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-cutouts-overlay"

#include "phoc-config.h"

#include "server.h"
#include "render-private.h"
#include "output-cutouts.h"

#include <gmobile.h>
#include <cairo/cairo.h>
#include <drm_fourcc.h>

G_DEFINE_AUTOPTR_CLEANUP_FUNC (cairo_t, cairo_destroy)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (cairo_surface_t, cairo_surface_destroy)

/**
 * PhocOutputCutouts:
 *
 * Tracks display cutout information. This can be used for providing
 * cutout information to clients and to render an overlay texture
 * showing the devices cutouts.
 */

enum {
  PROP_0,
  PROP_COMPATIBLES,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhocOutputCutouts {
  GObject parent;

  GStrv   compatibles;
  GmDisplayPanel *panel;

  pixman_region32_t cutouts;
  GArray *corners;
};
G_DEFINE_TYPE (PhocOutputCutouts, phoc_output_cutouts, G_TYPE_OBJECT)


static void
phoc_output_cutouts_update (PhocOutputCutouts *self)
{
  GListModel *cutouts;
  GArray *radii;

  pixman_region32_clear (&self->cutouts);

  if (self->panel == NULL)
    return;

  cutouts = gm_display_panel_get_cutouts (self->panel);
  for (int i = 0; i < g_list_model_get_n_items (cutouts); i++) {
    g_autoptr (GmCutout) cutout = g_list_model_get_item (cutouts, i);
    const GmRect *bounds = gm_cutout_get_bounds (cutout);

    pixman_region32_union_rect (&self->cutouts, &self->cutouts,
                                bounds->x, bounds->y, bounds->width, bounds->height);
  }

  radii = gm_display_panel_get_corner_radii (self->panel);
  g_array_remove_range (self->corners, 0, self->corners->len);
  for (int i = 0; i < PHOC_NUM_CORNERS; i++) {
    int radius = g_array_index (radii, int, i);
    PhocCutoutCorner corner = { .radius = radius, .position = i };

    g_array_append_val (self->corners, corner);
  }
}


static void
output_cutouts_set_compatibles (PhocOutputCutouts *self, const char *const *compatibles)
{
  GmDisplayPanel *panel = NULL;
  g_autoptr (GmDeviceInfo) info = NULL;

  if (compatibles == NULL) {
    g_strfreev (self->compatibles);
    return;
  }

  if (self->compatibles && g_strv_equal ((const char *const *)self->compatibles, compatibles))
    return;

  g_clear_pointer (&self->compatibles, g_strfreev);
  self->compatibles = g_strdupv ((GStrv)compatibles);
  info = gm_device_info_new ((const char * const *)self->compatibles);
  panel = gm_device_info_get_display_panel (info);

  if (panel == NULL)
    g_warning ("No panel found for compatibles");

  g_debug ("Found panel '%s'", gm_display_panel_get_name (panel));
  g_set_object (&self->panel, panel);

  phoc_output_cutouts_update (self);
}


static void
phoc_output_cutouts_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PhocOutputCutouts *self = PHOC_OUTPUT_CUTOUTS (object);

  switch (property_id) {
  case PROP_COMPATIBLES:
    output_cutouts_set_compatibles (self, g_value_get_boxed (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_output_cutouts_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PhocOutputCutouts *self = PHOC_OUTPUT_CUTOUTS (object);

  switch (property_id) {
  case PROP_COMPATIBLES:
    g_value_set_boxed (value, self->compatibles);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_output_cutouts_finalize (GObject *object)
{
  PhocOutputCutouts *self = PHOC_OUTPUT_CUTOUTS (object);

  g_clear_object (&self->panel);
  g_clear_pointer (&self->compatibles, g_strfreev);

  g_clear_pointer (&self->corners, g_array_unref);
  pixman_region32_fini (&self->cutouts);

  G_OBJECT_CLASS (phoc_output_cutouts_parent_class)->finalize (object);
}


static void
phoc_output_cutouts_class_init (PhocOutputCutoutsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_output_cutouts_get_property;
  object_class->set_property = phoc_output_cutouts_set_property;
  object_class->finalize = phoc_output_cutouts_finalize;

  props[PROP_COMPATIBLES] =
    g_param_spec_boxed ("compatibles", "", "",
                        G_TYPE_STRV,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_output_cutouts_init (PhocOutputCutouts *self)
{
  pixman_region32_init (&self->cutouts);
  self->corners = g_array_new (FALSE, FALSE, sizeof (PhocCutoutCorner));
}


PhocOutputCutouts *
phoc_output_cutouts_new (const char * const *compatibles)
{
  return g_object_new (PHOC_TYPE_OUTPUT_CUTOUTS,
                       "compatibles", compatibles,
                       NULL);
}


struct wlr_texture *
phoc_output_cutouts_get_cutouts_texture (PhocOutputCutouts *self)
{
  int width, height, stride;
  unsigned char *data;
  PhocServer *server = phoc_server_get_default ();
  PhocRenderer *renderer = phoc_server_get_renderer (server);
  struct wlr_texture *texture;
  g_autoptr (cairo_surface_t) surface = NULL;
  g_autoptr (cairo_t) cr = NULL;
  const pixman_box32_t *boxes;
  const PhocCutoutCorner *corner;
  int n_cutouts;

  if (self->panel == NULL)
    return NULL;

  width = gm_display_panel_get_x_res (self->panel);
  height = gm_display_panel_get_y_res (self->panel);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);
  cairo_set_line_width (cr, 5.0);
  cairo_set_source_rgba (cr, 0.5f, 0.0f, 0.5f, 0.5f);

  boxes = pixman_region32_rectangles (&self->cutouts, &n_cutouts);
  for (int i = 0; i < n_cutouts; i++) {
    cairo_rectangle (cr, boxes[i].x1, boxes[i].y1,
                     boxes[i].x2 - boxes[i].x1,
                     boxes[i].y2 - boxes[i].y1);
    cairo_fill (cr);
  }

  corner = phoc_output_cutouts_get_corner (self, PHOC_CORNER_TOP_LEFT);
  if (corner) {
    cairo_move_to (cr, 0, 0);
    cairo_arc (cr, corner->radius, corner->radius, corner->radius, M_PI, 1.5 * M_PI);
    cairo_close_path (cr);
    cairo_fill (cr);
  }

  corner = phoc_output_cutouts_get_corner (self, PHOC_CORNER_TOP_RIGHT);
  if (corner) {
    cairo_move_to (cr, width, 0);
    cairo_arc (cr, width - corner->radius, corner->radius, corner->radius, 1.5 * M_PI, 2 * M_PI);
    cairo_close_path (cr);
    cairo_fill (cr);
  }

  corner = phoc_output_cutouts_get_corner (self, PHOC_CORNER_BOTTOM_RIGHT);
  if (corner) {
    cairo_move_to (cr, width, height);
    cairo_arc (cr, width - corner->radius, height - corner->radius, corner->radius, 0, 0.5 * M_PI);
    cairo_close_path (cr);
    cairo_fill (cr);
  }

  corner = phoc_output_cutouts_get_corner (self, PHOC_CORNER_BOTTOM_LEFT);
  if (corner) {
    cairo_move_to (cr, 0, height);
    cairo_arc (cr, corner->radius, height - corner->radius, corner->radius, 0.5 * M_PI, M_PI);
    cairo_close_path (cr);
    cairo_fill (cr);
  }

  cairo_surface_flush (surface);
  data = cairo_image_surface_get_data (surface);
  stride = cairo_image_surface_get_stride (surface);

  texture = wlr_texture_from_pixels (phoc_renderer_get_wlr_renderer (renderer),
                                     DRM_FORMAT_ARGB8888, stride, width, height, data);

  return texture;
}

/**
 * phoc_output_cutouts_get_region:
 * @self: The cutouts
 *
 * Get cutouts region
 *
 * Returns: The cutouts region
 */
const pixman_region32_t *
phoc_output_cutouts_get_region (PhocOutputCutouts *self)
{
  g_assert (PHOC_IS_OUTPUT_CUTOUTS (self));

  return &self->cutouts;
}

/**
 * phoc_output_cutouts_get_corners:
 * @self: The cutouts
 *
 * If the panel has rounded corner, get corner cutouts.
 *
 * Returns:(element-type PhocCutoutCorner): The corner cutouts
 */
const GArray *
phoc_output_cutouts_get_corners (PhocOutputCutouts *self)
{
  g_assert (PHOC_IS_OUTPUT_CUTOUTS (self));

  return self->corners;
}

/**
 * phoc_output_cutouts_get_corner:
 * @self: The cutouts
 *
 * Get the rounded corner, if present.
 *
 * Returns:(nullable)(transfer none): The corner
 */
const PhocCutoutCorner *
phoc_output_cutouts_get_corner (PhocOutputCutouts *self, PhocCornerPosition pos)
{
  g_assert (PHOC_IS_OUTPUT_CUTOUTS (self));

  for (int i = 0; i < self->corners->len; i++) {
    PhocCutoutCorner *corner = &g_array_index (self->corners, PhocCutoutCorner, i);

    if (corner->position == pos)
      return corner;
  }

  return NULL;
}
