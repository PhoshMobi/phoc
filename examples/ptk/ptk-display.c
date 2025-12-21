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
#include "ptk-subsurface.h"
#include "ptk-toplevel.h"
#include "../egl-common.h"

#include "xdg-shell-client-protocol.h"

#include <glib.h>

#include <wayland-client.h>
#include <wayland-cursor.h>


typedef struct _PtkDisplayPrivate {
  struct wl_cursor_theme *cursor_theme;
  struct wl_surface      *cursor_surface;
  struct wl_cursor_image *cursor_image;

  PtkSurface *pointer_surface;
  gint32      pointer_x, pointer_y;
  GPtrArray  *toplevels;
} PtkDisplayPrivate;


static PtkDisplay display_;
static PtkDisplayPrivate display_private;


static void
wl_pointer_enter (void              *data,
                  struct wl_pointer *wl_pointer,
                  uint32_t           serial,
                  struct wl_surface *wl_surface,
                  wl_fixed_t         surface_x,
                  wl_fixed_t         surface_y)
{
  struct wl_cursor_image *image = display_private.cursor_image;
  PtkSurface *surface = NULL;

  wl_surface_attach (display_private.cursor_surface, wl_cursor_image_get_buffer (image), 0, 0);
  wl_surface_damage (display_private.cursor_surface, 1, 0, image->width, image->height);
  wl_surface_commit (display_private.cursor_surface);
  wl_pointer_set_cursor (wl_pointer,
                         serial,
                         display_private.cursor_surface,
                         image->hotspot_x,
                         image->hotspot_y);

  for (int i = 0; i < display_private.toplevels->len; i++) {
    PtkToplevel *toplevel = g_ptr_array_index (display_private.toplevels, i);
    PtkSubsurface *subsurface;

    if (PTK_SURFACE (toplevel)->surface == wl_surface) {
      surface = PTK_SURFACE (toplevel);
      break;
    }

    subsurface = ptk_surface_find_subsurface (PTK_SURFACE (toplevel), wl_surface);
    if (subsurface) {
      surface = PTK_SURFACE (subsurface);
      break;
    }
  }

  g_return_if_fail (surface);

  display_private.pointer_surface = surface;
}


static void
wl_pointer_leave (void              *data,
                  struct wl_pointer *wl_pointer,
                  uint32_t           serial,
                  struct wl_surface *surface)
{
  display_private.pointer_x = display_private.pointer_y = -1;
  display_private.pointer_surface = NULL;
}


static void
wl_pointer_motion (void              *data,
                   struct wl_pointer *wl_pointer,
                   uint32_t           time,
                   wl_fixed_t         surface_x,
                   wl_fixed_t         surface_y)
{
  display_private.pointer_x = wl_fixed_to_int (surface_x);
  display_private.pointer_y = wl_fixed_to_int (surface_y);
}


static void
wl_pointer_button (void              *data,
                   struct wl_pointer *wl_pointer,
                   uint32_t           serial,
                   uint32_t           time,
                   uint32_t           button,
                   uint32_t           state)
{
  PtkButtonEvent event = {
    .button = button,
    .state = state,
    .x = display_private.pointer_x,
    .y = display_private.pointer_y,
  };

  g_return_if_fail (display_private.pointer_surface);

  wl_signal_emit (&display_private.pointer_surface->events.clicked, &event);
}


static void
wl_pointer_axis (void              *data,
                 struct wl_pointer *wl_pointer,
                 uint32_t           time,
                 uint32_t           axis,
                 wl_fixed_t         value)
{
  /* Who cares */
}


static void
wl_pointer_frame (void *data, struct wl_pointer *wl_pointer)
{
  /* Who cares */
}


static void
wl_pointer_axis_source (void *data, struct wl_pointer *wl_pointer, uint32_t axis_source)
{
  /* Who cares */
}

static void
wl_pointer_axis_stop (void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis)
{
  /* Who cares */
}

static void
wl_pointer_axis_discrete (void              *data,
                          struct wl_pointer *wl_pointer,
                          uint32_t           axis,
                          int32_t            discrete)
{
  /* Who cares */
}


struct wl_pointer_listener pointer_listener = {
  .enter = wl_pointer_enter,
  .leave = wl_pointer_leave,
  .motion = wl_pointer_motion,
  .button = wl_pointer_button,
  .axis = wl_pointer_axis,
  .frame = wl_pointer_frame,
  .axis_source = wl_pointer_axis_source,
  .axis_stop = wl_pointer_axis_stop,
  .axis_discrete = wl_pointer_axis_discrete,
};


static void
touch_handle_down (void              *data,
                   struct wl_touch   *wl_touch,
                   uint32_t           serial,
                   uint32_t           time,
                   struct wl_surface *surface,
                   int32_t            id,
                   wl_fixed_t         x,
                   wl_fixed_t         y)
{
  g_debug ("%s", __func__);
}


static void
touch_handle_up (void            *data,
                 struct wl_touch *wl_touch_,
                 uint32_t         serial,
                 uint32_t         time,
                 int32_t          id)
{
  g_debug ("%s", __func__);
}


static void
touch_handle_motion (void            *data,
                     struct wl_touch *wl_touch,
                     uint32_t         time,
                     int32_t          id,
                     wl_fixed_t       x,
                     wl_fixed_t       y)
{
}


static void
touch_handle_frame (void *data, struct wl_touch *wl_touch_)
{
}


static void
touch_handle_cancel (void *data, struct wl_touch *wl_touch_)
{
  g_debug ("%s", __func__);
}


static void
touch_handle_shape (void            *data,
                    struct wl_touch *wl_touch,
                    int32_t          id,
                    wl_fixed_t       major,
                    wl_fixed_t       minor)
{
}


static void
touch_handle_orientation (void            *data,
                          struct wl_touch *wl_touch,
                          int32_t          id,
                          wl_fixed_t       orientation)
{
}


static struct wl_touch_listener touch_listener = {
  .down = touch_handle_down,
  .up = touch_handle_up,
  .motion = touch_handle_motion,
  .frame = touch_handle_frame,
  .cancel = touch_handle_cancel,
  .shape = touch_handle_shape,
  .orientation = touch_handle_orientation,
};


static void
wl_keyboard_keymap (void               *data,
                    struct wl_keyboard *wl_keyboard,
                    uint32_t            format,
                    int32_t             fd,
                    uint32_t            size)
{
  /* Who cares */
}


static void
wl_keyboard_enter (void               *data,
                   struct wl_keyboard *wl_keyboard,
                   uint32_t            serial,
                   struct wl_surface  *surface,
                   struct wl_array    *keys)
{
  g_debug ("Keyboard enter");
}


static void
wl_keyboard_leave (void               *data,
                   struct wl_keyboard *wl_keyboard,
                   uint32_t            serial,
                   struct wl_surface  *surface)
{
  g_debug ("Keyboard leave");
}


static void
wl_keyboard_key (void               *data,
                 struct wl_keyboard *wl_keyboard,
                 uint32_t            serial,
                 uint32_t            time,
                 uint32_t            key,
                 uint32_t            state)
{
  g_debug ("Key event: %d %d", key, state);
}


static void
wl_keyboard_modifiers (void               *data,
                       struct wl_keyboard *wl_keyboard,
                       uint32_t            serial,
                       uint32_t            mods_depressed,
                       uint32_t            mods_latched,
                       uint32_t            mods_locked,
                       uint32_t            group)
{
  /* Who cares */
}


static void
wl_keyboard_repeat_info (void               *data,
                         struct wl_keyboard *wl_keyboard,
                         int32_t             rate,
                         int32_t             delay)
{
  /* Who cares */
}


static struct wl_keyboard_listener keyboard_listener = {
  .keymap = wl_keyboard_keymap,
  .enter = wl_keyboard_enter,
  .leave = wl_keyboard_leave,
  .key = wl_keyboard_key,
  .modifiers = wl_keyboard_modifiers,
  .repeat_info = wl_keyboard_repeat_info,
};


static void
seat_handle_capabilities (void *data, struct wl_seat *wl_seat, enum wl_seat_capability caps)
{
  PtkDisplay *display = data;

  if ((caps & WL_SEAT_CAPABILITY_POINTER)) {
    display->pointer = wl_seat_get_pointer (wl_seat);
    wl_pointer_add_listener (display->pointer, &pointer_listener, display);
  }
  if ((caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
    display->keyboard = wl_seat_get_keyboard (wl_seat);
    wl_keyboard_add_listener (display->keyboard, &keyboard_listener, NULL);
  }
  if ((caps & WL_SEAT_CAPABILITY_TOUCH) && display->touch == NULL) {
    display->touch = wl_seat_get_touch (wl_seat);
    wl_touch_add_listener (display->touch, &touch_listener, NULL);
  }
}


static void
seat_handle_name (void *data, struct wl_seat *wl_seat, const char *name)
{
  /* Who cares */
}


const struct wl_seat_listener seat_listener = {
  .capabilities = seat_handle_capabilities,
  .name = seat_handle_name,
};


static void
xdg_wm_base_ping (void *data, struct xdg_wm_base *xdg_wm_base_, uint32_t serial)
{
  xdg_wm_base_pong (xdg_wm_base_, serial);
}


static const struct xdg_wm_base_listener xdg_wm_base_listener = {
  .ping = xdg_wm_base_ping,
};


static void
handle_global (void               *data,
               struct wl_registry *registry,
               uint32_t            name,
               const char         *interface,
               uint32_t            version)

{
  PtkDisplay *display = data;

  if (strcmp (interface, wl_compositor_interface.name) == 0) {
    display->compositor = wl_registry_bind (registry, name, &wl_compositor_interface, 1);
  } else if (strcmp (interface, wl_subcompositor_interface.name) == 0) {
    display->subcompositor = wl_registry_bind (registry, name, &wl_subcompositor_interface, 1);
  } else if (strcmp (interface, wl_shm_interface.name) == 0) {
    display->shm = wl_registry_bind (registry, name, &wl_shm_interface, 1);
  } else if (strcmp (interface, "wl_output") == 0) {
    if (!display->wl_output) {
      /* TODO: hande all outputs */
      display->wl_output = wl_registry_bind (registry, name, &wl_output_interface, 1);
    }
  } else if (strcmp (interface, wl_seat_interface.name) == 0) {
    display->seat = wl_registry_bind (registry, name, &wl_seat_interface, 1);
    wl_seat_add_listener (display->seat, &seat_listener, display);
  } else if (strcmp (interface, xdg_wm_base_interface.name) == 0) {
    display->xdg_wm_base = wl_registry_bind (registry, name, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener (display->xdg_wm_base, &xdg_wm_base_listener, NULL);
  }
}


static void
handle_global_remove (void *data, struct wl_registry *registry, uint32_t name)
{
  // who cares
}


static const struct wl_registry_listener registry_listener = {
  .global = handle_global,
  .global_remove = handle_global_remove,
};


static void
cursor_init (PtkDisplay *display)
{
  struct wl_cursor *cursor;

  display_private.cursor_theme = wl_cursor_theme_load (NULL, 16, display->shm);
  g_assert (display_private.cursor_theme);

  cursor = wl_cursor_theme_get_cursor (display_private.cursor_theme, "crosshair");
  if (!cursor)
    cursor = wl_cursor_theme_get_cursor (display_private.cursor_theme, "left_ptr");
  g_assert (cursor);
  display_private.cursor_image = cursor->images[0];

  display_private.cursor_surface = wl_compositor_create_surface (display->compositor);
  g_assert (display_private.cursor_surface);
}


PtkDisplay *
ptk_display_init (void)
{
  PtkDisplay *display = &display_;

  display->display = wl_display_connect (NULL);
  if (display->display == NULL) {
    g_critical ("Failed to create display");
    return NULL;
  }

  display->registry = wl_display_get_registry (display->display);
  wl_registry_add_listener (display->registry, &registry_listener, display);
  wl_display_roundtrip (display->display);

  if (display->compositor == NULL) {
    g_critical ("wl_compositor not available");
    return NULL;
  }
  if (display->subcompositor == NULL) {
    g_critical ("wl_subcompositor not available");
    return NULL;
  }
  if (display->shm == NULL) {
    g_critical ("wl_shm not available");
    return NULL;
  }

  if (!egl_init (display->display)) {
    g_critical ("Failed to init EGL");
    return NULL;
  }

  cursor_init (display);

  display_private.toplevels = g_ptr_array_new ();

  return display;
}


void
ptk_display_uninit (void)
{
  g_clear_pointer (&display_private.toplevels, g_ptr_array_unref);
}


PtkDisplay *
ptk_display_get_default (void)
{
  return &display_;
}


void
ptk_display_add_toplevel (PtkDisplay *self, PtkToplevel *toplevel)
{
  g_ptr_array_add (display_private.toplevels, toplevel);
}


void
ptk_display_remove_toplevel (PtkDisplay *self, PtkToplevel *toplevel)
{
  if (!g_ptr_array_remove (display_private.toplevels, toplevel))
    g_critical ("Toplevel %p does not exist", toplevel);
}
