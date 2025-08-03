/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-workspace-indicator"

#include "phoc-config.h"

#include "bling.h"
#include "cairo-texture.h"
#include "server.h"
#include "timed-animation.h"
#include "workspace-indicator.h"

#include <pango/pangocairo.h>
#include <cairo.h>

#define FADE_DURATION_MS 200
#define SHOW_DURATION_MS 200
#define FONT "Sans Bold 32"

/**
 * PhocWorkspaceIndicator:
 *
 * The workspace indicator
 */

enum {
  PROP_0,
  PROP_NUM,
  PROP_LX,
  PROP_LY,
  PROP_SIZE,
  PROP_ANIMATABLE,
  PROP_ALPHA,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhocWorkspaceIndicator {
  GObject             parent;

  int                 lx;
  int                 ly;

  PhocAnimatable     *animatable;
  PhocTimedAnimation *animation;
  PhocPropertyEaser  *easer;
  float               alpha;
  int                 num;
  int                 size;
  guint               timeout_id;
  PhocCairoTexture   *texture;
};

static void bling_interface_init (PhocBlingInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PhocWorkspaceIndicator, phoc_workspace_indicator, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (PHOC_TYPE_BLING, bling_interface_init))


static void
on_show_timeout (gpointer data)
{
  PhocWorkspaceIndicator *self = PHOC_WORKSPACE_INDICATOR (data);

  g_assert (PHOC_IS_WORKSPACE_INDICATOR (data));

  self->timeout_id = 0;
  phoc_bling_unmap (PHOC_BLING (self));
}


static void
on_animation_done (PhocWorkspaceIndicator *self)
{
  self->timeout_id = g_timeout_add_once (SHOW_DURATION_MS, on_show_timeout, self);
}


static PhocBox
bling_get_box (PhocBling *bling)
{
  PhocWorkspaceIndicator *self = PHOC_WORKSPACE_INDICATOR (bling);

  return (PhocBox) {
    .x = self->lx,
    .y = self->ly,
    .width = self->size,
    .height = self->size,
  };
}


static void
bling_render (PhocBling *bling, PhocRenderContext *ctx)
{
  PhocWorkspaceIndicator *self = PHOC_WORKSPACE_INDICATOR (bling);
  struct wlr_render_texture_options options;
  struct wlr_box box = bling_get_box (bling);
  pixman_region32_t damage;
  struct wlr_texture *texture = phoc_cairo_texture_get_texture (self->texture);

  if (!texture)
    return;

  box.x -= ctx->output->lx;
  box.y -= ctx->output->ly;
  phoc_utils_scale_box (&box, ctx->output->wlr_output->scale);
  phoc_output_transform_box (ctx->output, &box);

  if (!phoc_utils_is_damaged (&box, ctx->damage, NULL, &damage)) {
    pixman_region32_fini (&damage);
    return;
  }

  options = (struct wlr_render_texture_options) {
    .alpha   = &self->alpha,
    .texture = texture,
    .dst_box = box,
    .clip    = &damage,
    .transform = ctx->output->wlr_output->transform,
  };

  wlr_render_pass_add_texture (ctx->render_pass, &options);
}


static void
draw_indicator (cairo_t *cr, const char *text, double size)
{
  g_autoptr (PangoLayout) layout = NULL;
  g_autoptr (PangoFontDescription) desc = NULL;
  int layout_width, layout_height;
  double offset_x, offset_y, radius;
  const double degrees = M_PI / 180.0;

  cairo_save (cr);

  radius = size / 10.0;
  cairo_new_sub_path (cr);
  cairo_arc (cr, size - radius, radius, radius, -90 * degrees, 0 * degrees);
  cairo_arc (cr, size - radius, size - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, radius, size - radius, radius, 90 * degrees, 180 * degrees);
  cairo_arc (cr, radius, radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);

  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
  cairo_fill_preserve (cr);

  cairo_restore (cr);

  layout = pango_cairo_create_layout (cr);

  pango_layout_set_text (layout, text, -1);
  desc = pango_font_description_from_string (FONT);
  pango_layout_set_font_description (layout, desc);

  cairo_save (cr);

  cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);

  pango_cairo_update_layout (cr, layout);

  pango_layout_get_size (layout, &layout_width, &layout_height);

  offset_x = (size - (double)layout_width / PANGO_SCALE) / 2.0;
  offset_y = (size - (double)layout_height / PANGO_SCALE) / 2.0;
  cairo_move_to (cr, offset_x, offset_y);
  pango_cairo_show_layout (cr, layout);

  cairo_restore (cr);
}


static void
bling_map (PhocBling *bling)
{
  PhocWorkspaceIndicator *self = PHOC_WORKSPACE_INDICATOR (bling);
  PhocServer *server = phoc_server_get_default ();
  PhocDesktop *desktop = phoc_server_get_desktop (server);
  PhocOutput *output = phoc_desktop_layout_get_output (desktop, self->lx, self->ly);
  g_autofree char *text = NULL;
  float size = self->size;
  cairo_t *cr;

  if (output)
    size = ceil (size * phoc_output_get_scale (output));

  if (self->texture)
    return;

  self->texture = phoc_cairo_texture_new (size, size);
  cr = phoc_cairo_texture_get_context (self->texture);

  if (!cr) {
    g_warning ("No Cairo context, cannot render workspace indicator");
    return;
  }

  cairo_set_antialias (cr, CAIRO_ANTIALIAS_FAST);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

  text = g_strdup_printf ("%d", self->num);
  draw_indicator (cr, text, self->size);

  phoc_cairo_texture_update (self->texture);

  phoc_bling_damage_box (PHOC_BLING (self));
  phoc_timed_animation_play (self->animation);
}


static void
bling_unmap (PhocBling *bling)
{
  PhocWorkspaceIndicator *self = PHOC_WORKSPACE_INDICATOR (bling);

  if (!self->texture)
    return;

  phoc_bling_damage_box (PHOC_BLING (self));
  phoc_timed_animation_reset (self->animation);

  g_clear_object (&self->texture);
}


static gboolean
bling_is_mapped (PhocBling *bling)
{
  PhocWorkspaceIndicator *self = PHOC_WORKSPACE_INDICATOR (bling);

  return self->texture != NULL;
}


static void
bling_interface_init (PhocBlingInterface *iface)
{
  iface->get_box = bling_get_box;
  iface->render = bling_render;
  iface->map = bling_map;
  iface->unmap = bling_unmap;
  iface->is_mapped = bling_is_mapped;
}


static void
phoc_workspace_indicator_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  PhocWorkspaceIndicator *self = PHOC_WORKSPACE_INDICATOR (object);

  switch (property_id) {
  case PROP_NUM:
    self->num = g_value_get_int (value);
    break;
  case PROP_LX:
    phoc_bling_damage_box (PHOC_BLING (self));
    self->lx = g_value_get_int (value);
    phoc_bling_damage_box (PHOC_BLING (self));
    break;
  case PROP_LY:
    phoc_bling_damage_box (PHOC_BLING (self));
    self->ly = g_value_get_int (value);
    phoc_bling_damage_box (PHOC_BLING (self));
    break;
  case PROP_SIZE:
    self->size = g_value_get_int (value);
    break;
  case PROP_ANIMATABLE:
    self->animatable = g_value_get_object (value);
    break;
  case PROP_ALPHA:
    self->alpha = g_value_get_float (value);
    phoc_bling_damage_box (PHOC_BLING (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_workspace_indicator_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  PhocWorkspaceIndicator *self = PHOC_WORKSPACE_INDICATOR (object);

  switch (property_id) {
  case PROP_NUM:
    g_value_set_int (value, self->num);
    break;
  case PROP_LX:
    g_value_set_int (value, self->lx);
    break;
  case PROP_LY:
    g_value_set_int (value, self->ly);
    break;
  case PROP_SIZE:
    g_value_set_int (value, self->size);
    break;
  case PROP_ANIMATABLE:
    g_value_set_object (value, self->animatable);
    break;
  case PROP_ALPHA:
    g_value_set_float (value, self->alpha);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_workspace_indicator_constructed (GObject *object)
{
  PhocWorkspaceIndicator *self = PHOC_WORKSPACE_INDICATOR (object);

  G_OBJECT_CLASS (phoc_workspace_indicator_parent_class)->constructed (object);

  self->animation = g_object_new (PHOC_TYPE_TIMED_ANIMATION,
                                  "animatable", g_steal_pointer (&self->animatable),
                                  "duration", FADE_DURATION_MS,
                                  "property-easer", self->easer,
                                  NULL);
  g_signal_connect_swapped (self->animation, "done",
                            G_CALLBACK (on_animation_done),
                            self);
}


static void
phoc_workspace_indicator_finalize (GObject *object)
{
  PhocWorkspaceIndicator *self = PHOC_WORKSPACE_INDICATOR (object);

  g_clear_handle_id (&self->timeout_id, g_source_remove);

  phoc_bling_unmap (PHOC_BLING (self));
  g_clear_object (&self->easer);
  g_clear_object (&self->animation);

  G_OBJECT_CLASS (phoc_workspace_indicator_parent_class)->finalize (object);
}


static void
phoc_workspace_indicator_class_init (PhocWorkspaceIndicatorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_workspace_indicator_get_property;
  object_class->set_property = phoc_workspace_indicator_set_property;
  object_class->constructed = phoc_workspace_indicator_constructed;
  object_class->finalize = phoc_workspace_indicator_finalize;

  /**
   * PhocWorkspaceIndicator:animatable:
   *
   * A [type@Animatable] implementation that can drive this workspace indicator's animation
   */
  props[PROP_ANIMATABLE] =
    g_param_spec_object ("animatable", "", "",
                         PHOC_TYPE_ANIMATABLE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * PhocWorkspaceIndicator:num:
   *
   * The workspace number
   */
  props[PROP_NUM] =
    g_param_spec_int ("num", "", "",
                      0, INT32_MAX, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  /**
   * PhocWorkspaceIndicator:lx:
   *
   * The x coord to render workspace indicator at
   */
  props[PROP_LX] =
    g_param_spec_int ("lx", "", "",
                      0, INT32_MAX, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  /**
   * PhocWorkspaceIndicator:ly:
   *
   * The y coord to render workspace indicator at
   */
  props[PROP_LY] =
    g_param_spec_int ("ly", "", "",
                      0, INT32_MAX, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  /**
   * PhocWorkspaceIndicator:alpha:
   *
   * The current alpha of the workspace indicator.
   */
  props[PROP_ALPHA] =
    g_param_spec_float ("alpha", "", "",
                        0, 1.0, 1.0,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  /**
   * PhocWorkspaceIndicator:size:
   *
   * The width and height of the workspace indicator.
   */
  props[PROP_SIZE] =
    g_param_spec_int ("size", "", "",
                      16, 256, 128,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_workspace_indicator_init (PhocWorkspaceIndicator *self)
{
  self->easer = g_object_new (PHOC_TYPE_PROPERTY_EASER,
                              "target", self,
                              "easing", PHOC_EASING_NONE,
                              NULL);
  phoc_property_easer_set_props (self->easer,
                                 "alpha", 0.0, 1.0,
                                 NULL);
}


PhocWorkspaceIndicator *
phoc_workspace_indicator_new (PhocAnimatable *animatable, int num, int lx, int ly, int size)
{
  return g_object_new (PHOC_TYPE_WORKSPACE_INDICATOR,
                       "animatable", animatable,
                       "num", num,
                       "lx", lx,
                       "ly", ly,
                       "size", size,
                       NULL);
}
