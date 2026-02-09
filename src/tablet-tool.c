/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-tablet-tool"

#include "phoc-config.h"

#include "tablet-tool.h"
#include "cursor.h"
#include "server.h"


static void
handle_tablet_tool_destroy (struct wl_listener *listener, void *data)
{
  PhocTabletTool *self = wl_container_of (listener, self, tool_destroy);

  phoc_tablet_tool_free (self);
}


static void
handle_tablet_tool_set_cursor (struct wl_listener *listener, void *data)
{
  PhocTabletTool *self = wl_container_of (listener, self, set_cursor);
  struct wlr_tablet_v2_event_cursor *event = data;
  struct wlr_surface *focused_surface = event->seat_client->seat->pointer_state.focused_surface;
  struct wl_client *focused_client = NULL;
  gboolean has_focused = focused_surface != NULL && focused_surface->resource != NULL;

  phoc_seat_notify_activity (self->seat);

  if (has_focused)
    focused_client = wl_resource_get_client (focused_surface->resource);

  if (event->seat_client->client != focused_client ||
      phoc_cursor_get_mode (self->seat->cursor) != PHOC_CURSOR_PASSTHROUGH) {
    g_debug ("Denying request to set cursor from unfocused client");
    return;
  }

  phoc_cursor_set_image (self->seat->cursor,
                         focused_client,
                         event->surface,
                         event->hotspot_x,
                         event->hotspot_y);
}

/**
 * phoc_tablet_tool_new: (skip)
 * @wlr_tool: The wlr tablet tool
 * @seat: The tool's seat
 *
 * Build new tablet tool
 *
 * Returns:(transfer full): The tablet tool
 */
PhocTabletTool *
phoc_tablet_tool_new (struct wlr_tablet_tool *wlr_tool, PhocSeat *seat)
{
  PhocDesktop *desktop = phoc_server_get_desktop (phoc_server_get_default ());
  PhocTabletTool *self = g_new0 (PhocTabletTool, 1);

  self->seat = seat;
  wlr_tool->data = self;

  self->tablet_v2_tool = wlr_tablet_tool_create (desktop->tablet_v2,
                                                 seat->seat,
                                                 wlr_tool);

  self->tool_destroy.notify = handle_tablet_tool_destroy;
  wl_signal_add (&wlr_tool->events.destroy, &self->tool_destroy);

  self->set_cursor.notify = handle_tablet_tool_set_cursor;
  wl_signal_add (&self->tablet_v2_tool->events.set_cursor, &self->set_cursor);

  return self;
}


void
phoc_tablet_tool_free (PhocTabletTool *self)
{
  wl_list_remove (&self->tool_destroy.link);
  wl_list_remove (&self->set_cursor.link);

  g_free (self);
}


void
phoc_tablet_set_tilt_x (PhocTabletTool *self, double tilt_x)
{
  self->tilt_x = tilt_x;
}


void
phoc_tablet_set_tilt_y (PhocTabletTool *self, double tilt_y)
{
  self->tilt_y = tilt_y;
}
