/*
 * Copyright (C) 2023 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "view.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_VIEW_DECO (phoc_view_deco_get_type ())

G_DECLARE_FINAL_TYPE (PhocViewDeco, phoc_view_deco, PHOC, VIEW_DECO, GObject)

PhocViewDeco           *phoc_view_deco_new                       (PhocView *view);
PhocViewDecoPart        phoc_view_deco_get_part                  (PhocViewDeco *self, double sx, double sy);
guint                   phoc_view_deco_get_title_bar_height      (PhocViewDeco *self);
guint                   phoc_view_deco_get_border_width          (PhocViewDeco *self);

G_END_DECLS
