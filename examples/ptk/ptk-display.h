/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>

#include "xx-cutouts-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#include <wayland-client.h>

G_BEGIN_DECLS

#define PTK_POINTER_EVENT_BUTTON_LEFT 272
#define PTK_POINTER_EVENT_BUTTON_RIGHT 273

typedef struct _PtkButtonEvent {
  guint32 button;
  guint32 state;
  gint32  x,y;
} PtkButtonEvent;


typedef struct _PtkDisplay {
  struct wl_display            *display;
  struct wl_registry           *registry;
  struct wl_compositor         *compositor;
  struct wl_subcompositor      *subcompositor;
  struct wl_seat               *seat;
  struct wl_shm *shm;
  struct wl_pointer            *pointer;
  struct wl_keyboard           *keyboard;
  struct wl_touch              *touch;
  struct wl_output             *wl_output;
  struct xdg_wm_base           *xdg_wm_base;
  struct xx_cutouts_manager_v1 *xx_cutouts_manager_v1;
} PtkDisplay;

PtkDisplay *            ptk_display_init                        (void);
PtkDisplay *            ptk_display_get_default                 (void);
void                    ptk_display_uninit                      (void);

G_END_DECLS
