/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "../egl-common.h"

#include <glib.h>

#include <wayland-client.h>
#include <wayland-server.h>
#include <wayland-egl.h>

G_BEGIN_DECLS

typedef struct _PtkSurface {
  struct wl_surface    *surface;
  struct wl_egl_window *egl_window;
  EGLSurface *egl_surface;

  GPtrArray  *subsurfaces;

  uint32_t    width;
  uint32_t    height;

  struct wl_callback *frame_callback;

  struct {
    struct wl_signal frame;
    struct wl_signal clicked;
    struct wl_signal resized;
  } events;
} PtkSurface;

#define PTK_SURFACE(x) ((PtkSurface *)(x))


void                    ptk_surface_add_frame_listener            (PtkSurface         *surface,
                                                                   struct wl_listener *listener);

G_END_DECLS
