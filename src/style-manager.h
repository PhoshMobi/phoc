/*
 * Copyright (C) 2026 Phoc.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "phoc-types.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_STYLE_MANAGER (phoc_style_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhocStyleManager, phoc_style_manager, PHOC, STYLE_MANAGER, GObject)

PhocStyleManager *phoc_style_manager_get_default (void);
PhocColor         phoc_style_manager_get_accent_color (PhocStyleManager *self);

G_END_DECLS
