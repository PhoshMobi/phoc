/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ptk-display.h"
#include "ptk-toplevel.h"

#include <glib.h>

#include <wayland-client.h>

G_BEGIN_DECLS

void                    ptk_display_remove_toplevel             (PtkDisplay  *display,
                                                                 PtkToplevel *toplevel);
void                    ptk_display_add_toplevel                (PtkDisplay  *display,
                                                                 PtkToplevel *toplevel);

G_END_DECLS
