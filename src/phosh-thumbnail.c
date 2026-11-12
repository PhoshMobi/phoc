/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: nikodunk <n.dunkel@gmail.com>
 */

#define G_LOG_DOMAIN "phoc-phosh-thumbnail"

#include "phoc-config.h"

#include "phosh-thumbnail.h"

enum {
  PROP_0,
  PROP_VIEW,
  PROP_LAYER_SURFACE,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];

G_DEFINE_TYPE (PhocPhoshThumbnail, phoc_phosh_thumbnail, G_TYPE_OBJECT)


static void
on_view_surface_destroy (PhocPhoshThumbnail *self, PhocView *view)
{
  g_assert (PHOC_IS_VIEW (view));

  g_signal_handlers_disconnect_by_data (self->view, self);
  self->view = NULL;
}


static void
set_view (PhocPhoshThumbnail *self, PhocView *view)
{
  g_assert (self->view == NULL);

  /* We don't take ref but listen to `surface-destroy` instead */
  self->view = view;

  if (self->view == NULL)
    return;

  g_signal_connect_swapped (view,
                            "surface-destroy",
                            G_CALLBACK (on_view_surface_destroy),
                            self);
}


static void
on_layer_surface_destroy (PhocPhoshThumbnail *self, PhocLayerSurface *layer_surface)
{
  g_debug ("Thumbnail %p: layer surface %p went away", self, layer_surface);

  g_signal_handlers_disconnect_by_data (layer_surface, self);
  self->layer_surface = NULL;
}


static void
set_layer_surface (PhocPhoshThumbnail *self, PhocLayerSurface *layer_surface)
{
  g_assert (self->layer_surface == NULL);

  /* We don't take ref but listen to `destroy` instead */
  self->layer_surface = layer_surface;

  if (layer_surface == NULL)
    return;

  g_object_set_data (G_OBJECT (layer_surface), "phoc-thumbnail", self);
  g_signal_connect_swapped (layer_surface,
                            "destroy",
                            G_CALLBACK (on_layer_surface_destroy),
                            self);
}


static void
phoc_phosh_thumbnail_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  PhocPhoshThumbnail *self = PHOC_PHOSH_THUMBNAIL (object);

  switch (property_id) {
  case PROP_VIEW:
    set_view (self, g_value_get_object (value));
    break;
  case PROP_LAYER_SURFACE:
    set_layer_surface (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_phosh_thumbnail_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  PhocPhoshThumbnail *self = PHOC_PHOSH_THUMBNAIL (object);

  switch (property_id) {
  case PROP_VIEW:
    g_value_set_object (value, self->view);
    break;
  case PROP_LAYER_SURFACE:
    g_value_set_object (value, self->layer_surface);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_phosh_thumbnail_dispose (GObject *object)
{
  PhocPhoshThumbnail *self = PHOC_PHOSH_THUMBNAIL (object);

  if (self->view) {
    g_signal_handlers_disconnect_by_data (self->view, self);
    self->view = NULL;
  }

  if (self->layer_surface) {
    g_object_set_data (G_OBJECT (self->layer_surface), "phoc-thumbnail", NULL);
    g_signal_handlers_disconnect_by_data (self->layer_surface, self);
    self->layer_surface = NULL;
  }

  G_OBJECT_CLASS (phoc_phosh_thumbnail_parent_class)->dispose (object);
}


static void
phoc_phosh_thumbnail_class_init (PhocPhoshThumbnailClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_phosh_thumbnail_get_property;
  object_class->set_property = phoc_phosh_thumbnail_set_property;
  object_class->dispose = phoc_phosh_thumbnail_dispose;

  props[PROP_VIEW] =
    g_param_spec_object ("view", "", "",
                         PHOC_TYPE_VIEW,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  props[PROP_LAYER_SURFACE] =
    g_param_spec_object ("layer-surface", "", "",
                         PHOC_TYPE_LAYER_SURFACE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}


static void
phoc_phosh_thumbnail_init (PhocPhoshThumbnail *self)
{
  self->scale = 1.0;
  self->alpha = 1.0;
}


PhocPhoshThumbnail *
phoc_phosh_thumbnail_new (PhocView *view, PhocLayerSurface *layer_surface)
{
  return g_object_new (PHOC_TYPE_PHOSH_THUMBNAIL,
                       "view", view,
                       "layer-surface", layer_surface,
                       NULL);
}

/**
 * phoc_phosh_thumbnail_from_layer_surface:
 * @layer_surface: The layer surface
 *
 * Get the thumbnail attached to the given layer surface.
 *
 * Returns: (transfer none) (nullable): The thumbnail or %NULL if none
 */
PhocPhoshThumbnail *
phoc_phosh_thumbnail_from_layer_surface (PhocLayerSurface *layer_surface)
{
  g_return_val_if_fail (PHOC_IS_LAYER_SURFACE (layer_surface), NULL);

  return g_object_get_data (G_OBJECT (layer_surface), "phoc-thumbnail");
}
