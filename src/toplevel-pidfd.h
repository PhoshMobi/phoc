/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <wlr/types/wlr_ext_foreign_toplevel_list_v1.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_TOPLEVEL_PIDFD_MANAGER (phoc_toplevel_pidfd_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhocToplevelPidfdManager,
                      phoc_toplevel_pidfd_manager, PHOC, TOPLEVEL_PIDFD_MANAGER, GObject)

PhocToplevelPidfdManager *phoc_toplevel_pidfd_manager_new (void);
struct wl_global *phoc_toplevel_pidfd_manager_get_global (PhocToplevelPidfdManager *self);

G_END_DECLS
