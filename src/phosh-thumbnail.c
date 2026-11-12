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
