/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "seat.h"

#include <wlr/types/wlr_tablet_v2.h>
#include <wlr/types/wlr_tablet_tool.h>


G_BEGIN_DECLS

typedef struct _PhocTabletTool {
  struct wlr_tablet_v2_tablet_tool *tablet_v2_tool;

  PhocSeat *seat;
  double    tilt_x, tilt_y;

  struct wl_listener set_cursor;
  struct wl_listener tool_destroy;
} PhocTabletTool;

PhocTabletTool *phoc_tablet_tool_new (struct wlr_tablet_tool *wlr_tool, PhocSeat *seat);
void            phoc_tablet_tool_free (PhocTabletTool *self);
void            phoc_tablet_set_tilt_x (PhocTabletTool *self, double tilt_x);
void            phoc_tablet_set_tilt_y (PhocTabletTool *self, double tilt_y);

G_END_DECLS
