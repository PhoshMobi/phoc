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
#include "server.h"

#include <wlr/types/wlr_tablet_tool.h>
#include <wlr/types/wlr_tablet_pad.h>


typedef struct _PhocTabletPad {
  PhocInputDevice     parent;

  struct wlr_surface *current_surface;
  struct wlr_tablet_v2_tablet_pad *tablet_v2_pad;

  struct wl_listener  attach;
  struct wl_listener  button;
  struct wl_listener  ring;
  struct wl_listener  strip;
  struct wl_listener  surface_destroy;

  PhocTablet         *tablet;
  struct wl_listener  tablet_device_destroy;
} PhocTabletPad;


G_DEFINE_FINAL_TYPE (PhocTabletPad, phoc_tablet_pad, PHOC_TYPE_INPUT_DEVICE);


static void
handle_pad_tablet_device_destroy (struct wl_listener *listener, void *data)
{
  PhocTabletPad *self = wl_container_of (listener, self, tablet_device_destroy);

  self->tablet = NULL;

  wl_list_remove (&self->tablet_device_destroy.link);
  wl_list_init (&self->tablet_device_destroy.link);
}


static void
handle_tablet_pad_attach (struct wl_listener *listener, void *data)
{
  PhocTabletPad *self = wl_container_of (listener, self, attach);
  struct wlr_tablet_tool *wlr_tool = data;
  PhocTablet *tablet = PHOC_TABLET (wlr_tool->data);

  phoc_tablet_pad_attach_tablet (self, tablet);
}


static void
handle_tablet_pad_surface_destroy (struct wl_listener *listener, void *data)
{
  PhocTabletPad *self = wl_container_of (listener, self, surface_destroy);

  phoc_tablet_pad_set_focus (self, NULL);
}


static void
handle_tablet_pad_ring (struct wl_listener *listener, void *data)
{
  PhocTabletPad *pad = wl_container_of (listener, pad, ring);
  struct wlr_tablet_pad_ring_event *event = data;

  wlr_tablet_v2_tablet_pad_notify_ring (pad->tablet_v2_pad,
                                        event->ring, event->position,
                                        event->source == WLR_TABLET_PAD_RING_SOURCE_FINGER,
                                        event->time_msec);
}

static void
handle_tablet_pad_strip (struct wl_listener *listener, void *data)
{
  PhocTabletPad *pad = wl_container_of (listener, pad, strip);
  struct wlr_tablet_pad_strip_event *event = data;

  wlr_tablet_v2_tablet_pad_notify_strip (pad->tablet_v2_pad,
                                         event->strip, event->position,
                                         event->source == WLR_TABLET_PAD_STRIP_SOURCE_FINGER,
                                         event->time_msec);
}

static void
handle_tablet_pad_button (struct wl_listener *listener, void *data)
{
  PhocTabletPad *pad = wl_container_of (listener, pad, button);
  struct wlr_tablet_pad_button_event *event = data;

  wlr_tablet_v2_tablet_pad_notify_mode (pad->tablet_v2_pad,
                                        event->group, event->mode, event->time_msec);

  wlr_tablet_v2_tablet_pad_notify_button (pad->tablet_v2_pad,
                                          event->button, event->time_msec,
                                          (enum zwp_tablet_pad_v2_button_state)event->state);
}


static void
phoc_tablet_pad_constructed (GObject *object)
{
  PhocDesktop *desktop = phoc_server_get_desktop (phoc_server_get_default ());
  PhocTabletPad *self = PHOC_TABLET_PAD (object);
  PhocInputDevice *input_device = PHOC_INPUT_DEVICE (self);
  PhocSeat *seat = phoc_input_device_get_seat (input_device);
  struct wlr_input_device *device = phoc_input_device_get_device (input_device);
  struct wlr_tablet_pad *wlr_tablet_pad = wlr_tablet_pad_from_input_device (device);

  G_OBJECT_CLASS (phoc_tablet_pad_parent_class)->constructed (object);

  self->button.notify = handle_tablet_pad_button;
  wl_signal_add (&wlr_tablet_pad->events.button, &self->button);

  self->strip.notify = handle_tablet_pad_strip;
  wl_signal_add (&wlr_tablet_pad->events.strip, &self->strip);

  self->ring.notify = handle_tablet_pad_ring;
  wl_signal_add (&wlr_tablet_pad->events.ring, &self->ring);

  self->attach.notify = handle_tablet_pad_attach;
  wl_signal_add (&wlr_tablet_pad->events.attach_tablet, &self->attach);

  self->tablet_v2_pad = wlr_tablet_pad_create (desktop->tablet_v2, seat->seat, device);
}


static void
phoc_tablet_pad_finalize (GObject *object)
{
  PhocTabletPad *self = PHOC_TABLET_PAD (object);

  phoc_tablet_pad_set_focus (self, NULL);

  wl_list_remove (&self->attach.link);

  wl_list_remove (&self->button.link);
  wl_list_remove (&self->strip.link);
  wl_list_remove (&self->ring.link);

  wl_list_remove (&self->tablet_device_destroy.link);

  G_OBJECT_CLASS (phoc_tablet_pad_parent_class)->finalize (object);
}


static void
phoc_tablet_pad_class_init (PhocTabletPadClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phoc_tablet_pad_constructed;
  object_class->finalize = phoc_tablet_pad_finalize;
}


static void
phoc_tablet_pad_init (PhocTabletPad *self)
{
  wl_list_init (&self->tablet_device_destroy.link);
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


void
phoc_tablet_pad_attach_tablet (PhocTabletPad *self, PhocTablet *tablet)
{
  struct wlr_input_device *device;
  struct wlr_input_device *tablet_device;

  g_assert (PHOC_IS_TABLET_PAD (self));
  g_assert (PHOC_IS_TABLET (tablet));

  device = phoc_input_device_get_device (PHOC_INPUT_DEVICE (self));
  tablet_device = phoc_input_device_get_device (PHOC_INPUT_DEVICE (tablet));

  g_debug ("Attaching tablet pad '%s' to tablet '%s'", device->name, tablet_device->name);

  self->tablet = tablet;

  /* TODO: should use PhocInputDevice's "device-destroy" signal */
  wl_list_remove (&self->tablet_device_destroy.link);
  self->tablet_device_destroy.notify = handle_pad_tablet_device_destroy;
  wl_signal_add (&device->events.destroy, &self->tablet_device_destroy);
}
