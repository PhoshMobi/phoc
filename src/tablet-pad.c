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
#include "tablet-pad.h"


G_DEFINE_FINAL_TYPE (PhocTabletPad, phoc_tablet_pad, PHOC_TYPE_INPUT_DEVICE);


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
