/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ptk-surface.h"
#include "ptk-subsurface.h"


void                    ptk_surface_init                         (PtkSurface *self,
                                                                  uint32_t    width,
                                                                  uint32_t    height);
void                    ptk_surface_destroy                      (PtkSurface *self);

void                    ptk_surface_add_subsurface               (PtkSurface    *self,
                                                                  PtkSubsurface *subsurface);
void                    ptk_surface_remove_subsurface            (PtkSurface    *self,
                                                                  PtkSubsurface *subsurface);

PtkSubsurface *         ptk_surface_find_subsurface              (PtkSurface        *self,
                                                                  struct wl_surface *wl_surface);

void                    ptk_surface_resize                       (PtkSurface *self,
                                                                  uint32_t    width,
                                                                  uint32_t    height);

G_END_DECLS
