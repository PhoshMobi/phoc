/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <wlr/xwayland.h>

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
  PHOC_XWAYLAND_SURFACE_STATE_NONE = 0,
  PHOC_XWAYLAND_SURFACE_STATE_MAPPED = (1 << 0),
  PHOC_XWAYLAND_SURFACE_STATE_ASSOCIATED = (1 << 1),
} PhocXWaylandSurfaceState;

#define PHOC_TYPE_XWAYLAND_UNMANAGED (phoc_xwayland_unmanaged_get_type ())

G_DECLARE_FINAL_TYPE (PhocXWaylandUnmanaged, phoc_xwayland_unmanaged, PHOC, XWAYLAND_UNMANAGED,
                      GObject)

PhocXWaylandUnmanaged *phoc_xwayland_unmanaged_new (struct wlr_xwayland_surface *xwayland_surface,
                                                    PhocXWaylandSurfaceState     state);
gboolean               phoc_xwayland_unmanaged_is_mapped (PhocXWaylandUnmanaged *self);
void                   phoc_xwayland_unmanaged_get_pos (PhocXWaylandUnmanaged   *self,
                                                        int                     *lx,
                                                        int                     *ly);
void                   phoc_xwayland_unmanaged_damage_whole (PhocXWaylandUnmanaged *self);

struct wlr_surface *   phoc_xwayland_unmanaged_get_wlr_surface (PhocXWaylandUnmanaged *self);

G_END_DECLS
