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

#define PHOC_TYPE_TABLET_PAD (phoc_tablet_pad_get_type ())

G_DECLARE_FINAL_TYPE (PhocTabletPad, phoc_tablet_pad, PHOC, TABLET_PAD, PhocInputDevice);

PhocTabletPad *phoc_tablet_pad_new (struct wlr_input_device *device, PhocSeat *seat);
void           phoc_tablet_pad_set_focus (PhocTabletPad *self, struct wlr_surface *surface);
void           phoc_tablet_pad_attach_tablet (PhocTabletPad *self, PhocTablet *tablet);

G_END_DECLS
