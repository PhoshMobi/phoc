/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ptk-surface.h"

#include "xdg-shell-client-protocol.h"

#include <glib.h>

#include <wayland-client.h>

G_BEGIN_DECLS

typedef enum {
  PTK_TOPLEVEL_STATE_NONE,
  PTK_TOPLEVEL_STATE_FULLSCREEN,
  PTK_TOPLEVEL_STATE_MAXIMIZED,
} PtkToplevelState;

typedef struct _PtkToplevel {
  PtkSurface parent;

  struct xdg_surface  *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;

  PtkToplevelState     state;

  struct {
    /* Emitted on the end of a `configured` sequence */
    struct wl_signal configured;
  } events;
} PtkToplevel;


PtkToplevel *           ptk_toplevel_new                         (const char *title,
                                                                  guint32     width,
                                                                  guint32     height);
void                    ptk_toplevel_destroy                     (PtkToplevel *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PtkToplevel, ptk_toplevel_destroy)

G_END_DECLS
