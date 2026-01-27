/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "input-device.h"
#include "seat.h"

G_BEGIN_DECLS

/**
 * PhocTablet:
 *
 * A tablet input device
 */
typedef struct _PhocTablet {
  PhocInputDevice parent;

  struct wlr_tablet_v2_tablet *tablet_v2;
} PhocTablet;


typedef struct _PhocTabletTool {
  struct wl_list     link;
  struct wl_list     tool_link;
  struct wlr_tablet_v2_tablet_tool *tablet_v2_tool;

  PhocSeat          *seat;
  double             tilt_x, tilt_y;

  struct wl_listener set_cursor;
  struct wl_listener tool_destroy;

  PhocTablet        *current_tablet;
  struct wl_listener tablet_destroy;
} PhocTabletTool;


#define PHOC_TYPE_TABLET (phoc_tablet_get_type ())

G_DECLARE_FINAL_TYPE (PhocTablet, phoc_tablet, PHOC, TABLET, PhocInputDevice);

PhocTablet *phoc_tablet_new (struct wlr_input_device *device, PhocSeat *seat);

G_END_DECLS
