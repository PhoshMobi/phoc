/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ptk"

#include "ptk-display-private.h"
#include "ptk-surface-private.h"
#include "ptk-subsurface.h"


static void
ptk_subsurface_init (PtkSubsurface *self,
                     PtkSurface    *parent_surface,
                     PtkSurface    *root_surface,
                     guint32        x,
                     guint32        y)
{
  PtkDisplay *display = ptk_display_get_default ();
  PtkSurface *surface = PTK_SURFACE (self);

  self->parent_surface = parent_surface;
  self->root_surface = root_surface;
  ptk_surface_add_subsurface (parent_surface, self);

  self->subsurface = wl_subcompositor_get_subsurface (display->subcompositor,
                                                      surface->surface,
                                                      parent_surface->surface);
  wl_subsurface_set_position (self->subsurface, x, y);

  wl_surface_commit (PTK_SURFACE (self)->surface);

  wl_display_roundtrip (display->display);
}


static void
ptk_subsurface_finalize (PtkSubsurface *self)
{
  ptk_surface_remove_subsurface (self->parent_surface, self);
  ptk_surface_destroy (PTK_SURFACE (self));
}



PtkSubsurface *
ptk_subsurface_new (PtkSurface *parent_surface,
                    PtkSurface *root_surface,
                    guint32     x,
                    guint32     y,
                    guint32     width,
                    guint32     height)
{
  PtkSubsurface *self = g_new0 (PtkSubsurface, 1);

  ptk_surface_init (PTK_SURFACE (self), width, height);
  ptk_subsurface_init (self, parent_surface, root_surface, x, y);

  return self;
}


void
ptk_subsurface_destroy (PtkSubsurface *self)
{
  ptk_subsurface_finalize (self);
}


void
ptk_subsurface_map (PtkSubsurface *self)
{
  PtkSubsurface *subsurface = self;

  while (subsurface->parent_surface != subsurface->root_surface) {
    PtkSurface *parent = subsurface->parent_surface;

    wl_surface_commit (parent->surface);
    subsurface = PTK_SUBSURFACE (subsurface->parent_surface);
  }

  wl_surface_commit (self->root_surface->surface);
}


void
ptk_subsurface_place (PtkSubsurface *self, PtkSurface *sibling, gboolean above)
{
  PtkSurface *surface = NULL;

  /* Surface below root surface */
  if (self->root_surface == sibling) {
    surface = self->root_surface;
  } else {
    /* Sibling is another subsurface */
    PtkSubsurface *subsurface = PTK_SUBSURFACE (sibling);
    if (self->root_surface == subsurface->root_surface)
      surface = sibling;
  }

  g_assert (surface);
  if (above)
    wl_subsurface_place_above (self->subsurface, surface->surface);
  else
    wl_subsurface_place_below (self->subsurface, surface->surface);
}
