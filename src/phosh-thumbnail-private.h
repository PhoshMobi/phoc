/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "phosh-thumbnail.h"

G_BEGIN_DECLS

PhocPhoshThumbnail *phoc_phosh_thumbnail_new (PhocView *view, PhocLayerSurface *layer_surface);
void                phoc_phosh_thumbnail_set_props (PhocPhoshThumbnail* self,
                                                    float               x,
                                                    float               y,
                                                    float               scale,
                                                    float               alpha);

G_END_DECLS
