/*
 * Copyright (C) 2022-2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "output.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_OUTPUT_CUTOUTS (phoc_output_cutouts_get_type ())

G_DECLARE_FINAL_TYPE (PhocOutputCutouts, phoc_output_cutouts, PHOC, OUTPUT_CUTOUTS, GObject)

PhocOutputCutouts *phoc_output_cutouts_new (const char * const *compatibles);
struct wlr_texture *phoc_output_cutouts_get_cutouts_texture (PhocOutputCutouts *self,
                                                             PhocOutput         *output);

G_END_DECLS
