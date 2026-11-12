/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "view.h"
#include "layer-surface.h"

G_BEGIN_DECLS

#define PHOC_TYPE_PHOSH_THUMBNAIL (phoc_phosh_thumbnail_get_type ())

G_DECLARE_FINAL_TYPE (PhocPhoshThumbnail, phoc_phosh_thumbnail, PHOC, PHOSH_THUMBNAIL, GObject)

/**
 * PhocPhoshThumbnail:
 *
 * A compositor rendered thumbnail of a [type@View] attached to a
 * [type@LayerSurface].
 *
 * The view's content is rendered onto the layer surface (which acts as a
 * depth/position anchor) at the transform provided by the client.
 */
typedef struct _PhocPhoshThumbnail {
  GObject           parent;

  PhocView         *view;
  PhocLayerSurface *layer_surface;

  float x;
  float y;
  float scale;
  float alpha;
} PhocPhoshThumbnail;

PhocPhoshThumbnail *phoc_phosh_thumbnail_from_layer_surface (PhocLayerSurface *layer_surface);

float               phoc_phosh_thumbnail_get_x (PhocPhoshThumbnail* self);
float               phoc_phosh_thumbnail_get_y (PhocPhoshThumbnail* self);
float               phoc_phosh_thumbnail_get_alpha (PhocPhoshThumbnail* self);
float               phoc_phosh_thumbnail_get_scale (PhocPhoshThumbnail* self);
PhocView *          phoc_phosh_thumbnail_get_view (PhocPhoshThumbnail* self);
PhocLayerSurface *  phoc_phosh_thumbnail_get_layer_surface (PhocPhoshThumbnail* self);

G_END_DECLS
