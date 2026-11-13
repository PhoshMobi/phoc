/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <wlr/types/wlr_ext_foreign_toplevel_list_v1.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_TOPLEVEL_IDENTIFIER_MANAGER (phoc_toplevel_identifier_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhocToplevelIdentifierManager,
                      phoc_toplevel_identifier_manager, PHOC, TOPLEVEL_IDENTIFIER_MANAGER, GObject)

PhocToplevelIdentifierManager *phoc_toplevel_identifier_manager_new (void);
struct wl_global *phoc_toplevel_identifier_manager_get_global (PhocToplevelIdentifierManager *self);

G_END_DECLS
