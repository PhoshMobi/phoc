/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <wlr/types/wlr_xdg_dialog_v1.h>

#include <glib.h>

#pragma once

G_BEGIN_DECLS

void phoc_handle_xdg_new_dialog (struct wl_listener *listener, void *data);

G_END_DECLS
