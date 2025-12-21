/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "view.h"
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _PhocXxCutouts PhocXxCutouts;

G_DECLARE_FINAL_TYPE (PhocXxCutoutsManager, phoc_xx_cutouts_manager, PHOC, XX_CUTOUTS_MANAGER,
                      GObject)

struct _PhocXxCutouts {
  struct wl_resource   *resource;
  PhocView *view;
  PhocXxCutoutsManager *xx_cutouts_manager;

  GArray *pending_unhandled;
  GArray *current_unhandled;

  struct wl_listener xdg_toplevel_handle_destroy;
  struct wl_listener xdg_surface_handle_configure;
  struct wl_listener xdg_surface_handle_ack_configure;

  struct _PhocXxCutoutsEvents {
    struct wl_signal unhandled_updated;
  } events;
};

#define PHOC_TYPE_XX_CUTOUTS_MANAGER (phoc_xx_cutouts_manager_get_type ())

PhocXxCutoutsManager *phoc_xx_cutouts_manager_new            (void);
GSList *              phoc_xx_cutouts_manager_get_cutouts    (PhocXxCutoutsManager *self);

G_END_DECLS
