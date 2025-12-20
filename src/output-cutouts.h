/*
 * Copyright (C) 2022-2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "output.h"

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
  PHOC_CORNER_TOP_LEFT = 0,
  PHOC_CORNER_TOP_RIGHT = 1,
  PHOC_CORNER_BOTTOM_RIGHT = 2,
  PHOC_CORNER_BOTTOM_LEFT = 3,
  PHOC_NUM_CORNERS = 4,
} PhocCornerPosition;

typedef struct {
  PhocCornerPosition position;
  uint32_t radius;
} PhocCutoutCorner;

#define PHOC_TYPE_OUTPUT_CUTOUTS (phoc_output_cutouts_get_type ())

G_DECLARE_FINAL_TYPE (PhocOutputCutouts, phoc_output_cutouts, PHOC, OUTPUT_CUTOUTS, GObject)

PhocOutputCutouts *      phoc_output_cutouts_new (const char * const *compatibles);
struct wlr_texture *     phoc_output_cutouts_get_cutouts_texture (PhocOutputCutouts *self);
const pixman_region32_t *phoc_output_cutouts_get_region (PhocOutputCutouts *self);
const GArray *           phoc_output_cutouts_get_corners (PhocOutputCutouts *self);
const PhocCutoutCorner * phoc_output_cutouts_get_corner (PhocOutputCutouts *self,
                                                         PhocCornerPosition pos);

G_END_DECLS
