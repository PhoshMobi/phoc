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


void
ptk_surface_init (PtkSurface *self, uint32_t width, uint32_t height)
{
  PtkDisplay *display = ptk_display_get_default ();

  g_return_if_fail (width > 0 && height > 0);

  wl_signal_init (&self->events.clicked);
  wl_signal_init (&self->events.frame);
  wl_signal_init (&self->events.resized);

  self->width = width;
  self->height = height;

  self->surface = wl_compositor_create_surface (display->compositor);
  g_assert (self->surface);

  self->subsurfaces = g_ptr_array_new ();

  self->egl_window = wl_egl_window_create (self->surface, width, height);
  g_assert (self->egl_window);
  self->egl_surface = eglCreatePlatformWindowSurfaceEXT (egl_display,
                                                         egl_config,
                                                         self->egl_window,
                                                         NULL);
  g_assert (self->egl_surface != EGL_NO_SURFACE);
}


void
ptk_surface_destroy (PtkSurface *self)
{
  g_ptr_array_unref (self->subsurfaces);
}


void
ptk_surface_add_subsurface (PtkSurface *self, PtkSubsurface *subsurface)
{
  g_ptr_array_add (self->subsurfaces, subsurface);
}


void
ptk_surface_remove_subsurface (PtkSurface *self, PtkSubsurface *subsurface)
{
  g_assert (g_ptr_array_remove (self->subsurfaces, subsurface));
}


PtkSubsurface *
ptk_surface_find_subsurface (PtkSurface *self, struct wl_surface *wl_surface)
{
  for (int i = 0; i < self->subsurfaces->len; i++) {
    PtkSubsurface *subsurface = g_ptr_array_index (self->subsurfaces, i);

    if (PTK_SURFACE (subsurface)->surface == wl_surface) {
      return subsurface;
    } else {
      subsurface = ptk_surface_find_subsurface (PTK_SURFACE (subsurface), wl_surface);
      if (subsurface)
        return subsurface;
    }
  }

  return NULL;
}


static void surface_frame_callback (void *data, struct wl_callback *cb, uint32_t time);

static struct wl_callback_listener frame_listener = {
  .done = surface_frame_callback
};


static void
surface_frame_callback (void *data, struct wl_callback *cb, uint32_t time)
{
  PtkSurface *self = data;

  wl_callback_destroy (self->frame_callback);

  if (!wl_list_empty (&self->events.frame.listener_list)) {
    self->frame_callback = wl_surface_frame (self->surface);
    wl_callback_add_listener (self->frame_callback, &frame_listener, self);
  }

  wl_signal_emit (&self->events.frame, GUINT_TO_POINTER (time));
}


void
ptk_surface_add_frame_listener (PtkSurface *self, struct wl_listener *listener)
{
  if (!self->frame_callback) {
    self->frame_callback = wl_surface_frame (self->surface);
    wl_callback_add_listener (self->frame_callback, &frame_listener, self);
  }

  wl_signal_add (&self->events.frame, listener);
}


void
ptk_surface_resize (PtkSurface *self, uint32_t width, uint32_t height)
{
  if (self->width == width && self->height == height)
    return;

  if (width == 0 && height == 0)
    return;

  self->width = width;
  self->height = height;

  wl_egl_window_resize (self->egl_window, self->width, self->height, 0, 0);
  wl_signal_emit (&self->events.resized, NULL);
}
