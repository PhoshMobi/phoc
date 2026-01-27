/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "input-device.h"
#include "seat.h"
#include "tablet.h"

#include <wlr/types/wlr_tablet_v2.h>

G_BEGIN_DECLS

typedef struct _PhocTabletPad {
  PhocInputDevice    parent;

  struct wlr_tablet_v2_tablet_pad *tablet_v2_pad;

  struct wl_listener device_destroy;
  struct wl_listener attach;
  struct wl_listener button;
  struct wl_listener ring;
  struct wl_listener strip;

  PhocTablet        *tablet;
  struct wl_listener tablet_destroy;
} PhocTabletPad;

#define PHOC_TYPE_TABLET_PAD (phoc_tablet_pad_get_type ())

G_DECLARE_FINAL_TYPE (PhocTabletPad, phoc_tablet_pad, PHOC, TABLET_PAD, PhocInputDevice);

PhocTabletPad *phoc_tablet_pad_new (struct wlr_input_device *device, PhocSeat *seat);

G_END_DECLS
