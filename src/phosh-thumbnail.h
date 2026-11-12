/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "layer-surface.h"

G_BEGIN_DECLS

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
  PhocView         *view;
  PhocLayerSurface *layer_surface;

  float x;
  float y;
  float scale;
  float alpha;
} PhocPhoshThumbnail;

PhocPhoshThumbnail *phoc_phosh_thumbnail_from_layer_surface (PhocLayerSurface *layer_surface);

G_END_DECLS
