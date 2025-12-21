/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ptk-surface.h"

#include <glib.h>

#include <wayland-client.h>

G_BEGIN_DECLS

typedef struct _PtkSubsurface {
  PtkSurface  parent;

  struct wl_subsurface *subsurface;
  PtkSurface *parent_surface;
  PtkSurface *root_surface;
} PtkSubsurface;

#define PTK_SUBSURFACE(x) ((PtkSubsurface *)(x))

PtkSubsurface *         ptk_subsurface_new                        (PtkSurface    *parent_surface,
                                                                   PtkSurface    *main_surface,
                                                                   guint32        x,
                                                                   guint32        y,
                                                                   guint32        width,
                                                                   guint32        height);
void                    ptk_subsurface_destroy                    (PtkSubsurface *self);
void                    ptk_subsurface_map                        (PtkSubsurface *self);
void                    ptk_subsurface_place                      (PtkSubsurface *self,
                                                                   PtkSurface    *sibling,
                                                                   gboolean       above);

G_END_DECLS
