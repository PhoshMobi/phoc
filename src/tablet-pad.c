/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-tablet-pad"

#include "phoc-config.h"

#include "input-device.h"
#include "tablet.h"
#include "tablet-pad.h"


G_DEFINE_FINAL_TYPE (PhocTabletPad, phoc_tablet_pad, PHOC_TYPE_INPUT_DEVICE);


static void
handle_tablet_pad_surface_destroy (struct wl_listener *listener, void *data)
{
  PhocTabletPad *self = wl_container_of (listener, self, surface_destroy);

  phoc_tablet_pad_set_focus (self, NULL);
}


static void
phoc_tablet_pad_class_init (PhocTabletPadClass *klass)
{
}


static void
phoc_tablet_pad_init (PhocTabletPad *self)
{
}


PhocTabletPad *
phoc_tablet_pad_new (struct wlr_input_device *device, PhocSeat *seat)
{
  return g_object_new (PHOC_TYPE_TABLET_PAD,
                       "device", device,
                       "seat", seat,
                       NULL);
}


void
phoc_tablet_pad_set_focus (PhocTabletPad *self, struct wlr_surface *wlr_surface)
{
  if (self == NULL)
    return;

  if (self->current_surface == wlr_surface)
    return;

  /* Leave current surface */
  if (self->current_surface) {
    wlr_tablet_v2_tablet_pad_notify_leave (self->tablet_v2_pad, self->current_surface);
    wl_list_remove (&self->surface_destroy.link);
    wl_list_init (&self->surface_destroy.link);
    self->current_surface = NULL;
  }

  if (self->tablet == NULL)
    return;

  if (wlr_surface == NULL || !wlr_surface_accepts_tablet_v2 (wlr_surface, self->tablet->tablet_v2))
    return;

  wlr_tablet_v2_tablet_pad_notify_enter (self->tablet_v2_pad, self->tablet->tablet_v2, wlr_surface);

  self->current_surface = wlr_surface;
  self->surface_destroy.notify = handle_tablet_pad_surface_destroy;
  wl_signal_add (&wlr_surface->events.destroy, &self->surface_destroy);
}
