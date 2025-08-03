/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_WORKSPACE_INDICATOR (phoc_workspace_indicator_get_type ())

G_DECLARE_FINAL_TYPE (PhocWorkspaceIndicator, phoc_workspace_indicator, PHOC, WORKSPACE_INDICATOR,
                      GObject)


PhocWorkspaceIndicator *phoc_workspace_indicator_new (PhocAnimatable *animatable,
                                                      int             num,
                                                      int             lx,
                                                      int             ly,
                                                      int             size);

G_END_DECLS
