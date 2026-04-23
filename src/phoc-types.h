/*
 * Copyright (C) 2023-2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <glib-object.h>
#include <wlr/util/box.h>

#pragma once

G_BEGIN_DECLS

#define PHOC_TYPE_BOX (phoc_box_get_type ())

/**
 * PHOC_HEX_COLOR:
 * @r: The red component
 * @g: The green component
 * @b: The blue compoent
 *
 * Convenience macros to create a color from components in the [0x0..0xff] range rather than
 * [0.0, 1.0].
 */
#define PHOC_HEX_COLOR(r,g,b) ((PhocColor){((float)(r))/0xff,   \
                                           ((float)(g))/0xff,   \
                                           ((float)(b))/0xff,   \
                                           1.0})

typedef struct wlr_box PhocBox;

GType                   phoc_box_get_type                        (void) G_GNUC_CONST;
PhocBox *               phoc_box_copy                            (const PhocBox *box);
void                    phoc_box_free                            (PhocBox *box);

#define PHOC_TYPE_COLOR (phoc_color_get_type ())

typedef struct _PhocColor {
  float red;
  float green;
  float blue;
  float alpha;
} PhocColor;

GType                   phoc_color_get_type                      (void) G_GNUC_CONST;
PhocColor *             phoc_color_copy                          (const PhocColor *color);
void                    phoc_color_free                          (PhocColor *color);
gboolean                phoc_color_is_equal                      (PhocColor *c1, PhocColor *c2);

G_END_DECLS
