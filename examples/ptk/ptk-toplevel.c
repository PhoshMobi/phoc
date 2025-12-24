/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ptk"

#include "ptk-display-private.h"
#include "ptk-surface-private.h"
#include "ptk-toplevel.h"


static void
xdg_toplevel_handle_configure (void                *data,
                               struct xdg_toplevel *xdg_toplevel,
                               int32_t              width,
                               int32_t              height,
                               struct wl_array     *states)
{
  PtkToplevel *self = data;
  PtkSurface *surface = &self->parent;
  uint32_t *p;

  self->state = PTK_TOPLEVEL_STATE_NONE;
  wl_array_for_each (p, states) {
    uint32_t state = *p;
    switch (state) {
    case XDG_TOPLEVEL_STATE_FULLSCREEN:
      self->state |= PTK_TOPLEVEL_STATE_FULLSCREEN;
      break;
    case XDG_TOPLEVEL_STATE_MAXIMIZED:
      self->state |= PTK_TOPLEVEL_STATE_MAXIMIZED;
      break;
    default:
      break;
    }
  }

  g_debug ("Configured %p, size: %dx%d", xdg_toplevel, width, height);

  ptk_surface_resize (surface, width, height);
}


static void
xdg_toplevel_handle_close (void *data, struct xdg_toplevel *xdg_surface)
{
  /* TBD */
}


static const struct xdg_toplevel_listener xdg_toplevel_listener = {
  xdg_toplevel_handle_configure,
  xdg_toplevel_handle_close,
};


static void
xdg_surface_configure (void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
  PtkToplevel *self = data;

  xdg_surface_ack_configure (xdg_surface, serial);

  wl_signal_emit (&self->events.configured, NULL);
}


static const struct xdg_surface_listener xdg_surface_listener = {
  .configure = xdg_surface_configure,
};


static void
ptk_toplevel_init (PtkToplevel *self, const char *title)
{
  PtkDisplay *display = ptk_display_get_default ();

  wl_signal_init (&self->events.configured);

  self->xdg_surface = xdg_wm_base_get_xdg_surface (display->xdg_wm_base,
                                                   PTK_SURFACE (self)->surface);
  xdg_surface_add_listener (self->xdg_surface, &xdg_surface_listener, self);
  self->xdg_toplevel = xdg_surface_get_toplevel (self->xdg_surface);
  xdg_toplevel_add_listener (self->xdg_toplevel, &xdg_toplevel_listener, self);

  if (title)
    xdg_toplevel_set_title (self->xdg_toplevel, title);

  wl_surface_commit (PTK_SURFACE (self)->surface);

  wl_display_roundtrip (display->display);

  ptk_display_add_toplevel (display, self);
}


static void
ptk_toplevel_finalize (PtkToplevel *self)
{
  PtkDisplay *display = ptk_display_get_default ();

  ptk_display_remove_toplevel (display, self);
  ptk_surface_destroy (PTK_SURFACE (self));
}


PtkToplevel *
ptk_toplevel_new (const char *title, guint32 width, guint32 height)
{
  PtkToplevel *self = g_new0 (PtkToplevel, 1);

  ptk_surface_init (PTK_SURFACE (self), width, height);
  ptk_toplevel_init (self, title);

  return self;
}


void
ptk_toplevel_destroy (PtkToplevel *self)
{
  ptk_toplevel_finalize (self);

  g_free (self);
}
