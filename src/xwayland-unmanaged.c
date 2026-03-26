/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-xwayland-unmanaged"

#include "phoc-config.h"
#include "phoc-enums.h"

#include "server.h"
#include "xwayland-unmanaged.h"

/**
 * PhocXWaylandUnmanaged:
 *
 * An unmanaged (override_redirect) XWayland surface
 */

enum {
  PROP_0,
  PROP_WLR_XWAYLAND_SURFACE,
  PROP_SURFACE_STATE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhocXWaylandUnmanaged {
  GObject parent;

  struct wlr_xwayland_surface *wlr_xwayland_surface;
  PhocXWaylandSurfaceState state;
  int     lx, ly;

  struct wl_listener destroy;
  struct wl_listener associate;
  struct wl_listener dissociate;
  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener request_activate;
  struct wl_listener request_configure;
  struct wl_listener set_geometry;
  struct wl_listener set_override_redirect;

  struct wl_listener surface_commit;
};

G_DEFINE_TYPE (PhocXWaylandUnmanaged, phoc_xwayland_unmanaged, G_TYPE_OBJECT)

static void
phoc_xwayland_unmanaged_damage (PhocXWaylandUnmanaged *self, gboolean whole)
{
  PhocDesktop *desktop = phoc_server_get_desktop (phoc_server_get_default ());
  PhocOutput *output;

  if (!self || !phoc_xwayland_unmanaged_is_mapped (self))
    return;

  wl_list_for_each (output, &desktop->outputs, link) {
    struct wlr_box output_box;
    wlr_output_layout_get_box (desktop->layout, output->wlr_output, &output_box);
    phoc_output_damage_from_surface (output,
                                     self->wlr_xwayland_surface->surface,
                                     self->lx - output_box.x,
                                     self->ly - output_box.y,
                                     whole);
  }
}


static struct wlr_box
phoc_xwayland_unmanaged_get_box (PhocXWaylandUnmanaged *self)
{
  return (struct wlr_box) {
    self->lx,
    self->ly,
    self->wlr_xwayland_surface->width,
    self->wlr_xwayland_surface->height
  };
}


/* {{{ wlr_xwayland_surface signal handlers */

static void
handle_surface_commit (struct wl_listener *listener, void *data)
{
  PhocXWaylandUnmanaged *self = wl_container_of (listener, self, surface_commit);

  phoc_xwayland_unmanaged_damage (self, FALSE);
}


static void
handle_request_configure (struct wl_listener *listener, void *data)
{
  PhocXWaylandUnmanaged *self = wl_container_of (listener, self, request_configure);
  struct wlr_xwayland_surface *wlr_xwayland_surface = self->wlr_xwayland_surface;
  struct wlr_xwayland_surface_configure_event *ev = data;

  wlr_xwayland_surface_configure (wlr_xwayland_surface, ev->x, ev->y, ev->width, ev->height);
}


static void
set_wlr_xwayland_surface (PhocXWaylandUnmanaged       *self,
                          struct wlr_xwayland_surface *wlr_xwayland_surface)
{
  self->wlr_xwayland_surface = wlr_xwayland_surface;
  self->wlr_xwayland_surface->data = self;
}


static void
handle_set_geometry (struct wl_listener *listener, void *data)
{
  PhocXWaylandUnmanaged *self = wl_container_of (listener, self, set_geometry);
  struct wlr_xwayland_surface *wlr_xwayland_surface = self->wlr_xwayland_surface;

  g_debug ("Setting unmanaged geometry to %d,%d", self->lx, self->ly);

  if (self->lx == wlr_xwayland_surface->x && self->ly == wlr_xwayland_surface->y)
    return;

  phoc_xwayland_unmanaged_damage (self, TRUE);

  self->lx = self->wlr_xwayland_surface->x;
  self->ly = self->wlr_xwayland_surface->y;

  phoc_xwayland_unmanaged_damage (self, TRUE);
}


static void
handle_map (struct wl_listener *listener, void *data)
{
  PhocXWaylandUnmanaged *self = wl_container_of (listener, self, map);
  struct wlr_xwayland_surface *wlr_xwayland_surface = self->wlr_xwayland_surface;
  PhocDesktop *desktop = phoc_server_get_desktop (phoc_server_get_default ());

  self->lx = self->wlr_xwayland_surface->x;
  self->ly = self->wlr_xwayland_surface->y;
  phoc_desktop_insert_unmanaged (desktop, self);

  g_debug ("Mapping unmanaged surface %p at %d,%d", self, self->lx, self->ly);

  wl_signal_add (&wlr_xwayland_surface->events.set_geometry, &self->set_geometry);
  self->set_geometry.notify = handle_set_geometry;

  self->surface_commit.notify = handle_surface_commit;
  wl_signal_add (&wlr_xwayland_surface->surface->events.commit, &self->surface_commit);

  self->state |= PHOC_XWAYLAND_SURFACE_STATE_MAPPED;

  /* Move focus when not a popover, menu or similar */
  if (wlr_xwayland_surface_override_redirect_wants_focus (wlr_xwayland_surface)) {
    PhocSeat *seat;

    g_debug ("Focussing unmanaged surface %p", self);
    seat = phoc_server_get_last_active_seat (phoc_server_get_default ());
    wlr_xwayland_set_seat (desktop->xwayland, seat->seat);
    phoc_seat_set_focus_surface (seat, wlr_xwayland_surface->surface);
  }

  phoc_xwayland_unmanaged_damage (self, TRUE);

  PhocOutput *output;
  struct wlr_box box = phoc_xwayland_unmanaged_get_box (self);
  wl_list_for_each (output, &desktop->outputs, link) {
    bool intersects;

    intersects = wlr_output_layout_intersects (desktop->layout, output->wlr_output, &box);
    if (intersects)
      phoc_utils_wlr_surface_enter_output (wlr_xwayland_surface->surface, output->wlr_output);
  }
}


static void
handle_unmap (struct wl_listener *listener, void *data)
{
  PhocXWaylandUnmanaged *self = wl_container_of (listener, self, unmap);
  struct wlr_xwayland_surface *wlr_xwayland_surface = self->wlr_xwayland_surface;
  PhocServer *server = phoc_server_get_default ();
  PhocDesktop *desktop = phoc_server_get_desktop (server);
  PhocInput *input = phoc_server_get_input (server);

  g_assert (wlr_xwayland_surface->data == self);

  phoc_xwayland_unmanaged_damage (self, TRUE);

  wl_list_remove (&self->set_geometry.link);
  wl_list_remove (&self->surface_commit.link);

  self->state &= ~PHOC_XWAYLAND_SURFACE_STATE_MAPPED;

  if (phoc_desktop_remove_unmanaged (desktop, self))
    g_debug ("Unmapping unmanaged surface %p", self);
  else
    g_critical ("Unmanaged surface %p not found", self);

  PhocSeat *seat = phoc_input_get_last_active_seat (input);
  if (seat->seat->keyboard_state.focused_surface == wlr_xwayland_surface->surface) {

    /* This returns focus to the parent surface if there's one available. */
    /* Sway does this to handle JetBrains issues */
    if (wlr_xwayland_surface->parent && wlr_xwayland_surface->parent->surface
        && wlr_xwayland_surface_override_redirect_wants_focus (wlr_xwayland_surface->parent)) {
      phoc_seat_set_focus_surface (seat, wlr_xwayland_surface->parent->surface);
    } else {
      /* TODO: Restore focus to previous view */
      g_critical ("FIXME: should restore focus to previously focused view");
    }
  }
}


static void
handle_request_activate (struct wl_listener *listener, void *data)
{
  PhocXWaylandUnmanaged *self = wl_container_of (listener, self, request_activate);
  struct wlr_xwayland_surface *wlr_xwayland_surface = self->wlr_xwayland_surface;
  PhocView *view;
  PhocSeat *seat;
  pid_t pid;

  if (wlr_xwayland_surface->surface == NULL || !wlr_xwayland_surface->surface->mapped)
    return;

  seat = phoc_server_get_last_active_seat (phoc_server_get_default ());
  view = phoc_seat_get_focus_view (seat);
  if (!view)
    return;

  pid = phoc_view_get_pid (view);
  if (pid != wlr_xwayland_surface->pid)
    return;

  phoc_seat_set_focus_surface (seat, wlr_xwayland_surface->surface);
}


static void
handle_associate (struct wl_listener *listener, void *data)
{
  PhocXWaylandUnmanaged *self = wl_container_of (listener, self, associate);
  struct wlr_xwayland_surface *wlr_xwayland_surface = self->wlr_xwayland_surface;

  wl_signal_add (&wlr_xwayland_surface->surface->events.map, &self->map);
  self->map.notify = handle_map;
  wl_signal_add (&wlr_xwayland_surface->surface->events.unmap, &self->unmap);
  self->unmap.notify = handle_unmap;

  self->state |= PHOC_XWAYLAND_SURFACE_STATE_ASSOCIATED;
}


static void
handle_dissociate (struct wl_listener *listener, void *data)
{
  PhocXWaylandUnmanaged *self = wl_container_of (listener, self, dissociate);

  self->state &= ~PHOC_XWAYLAND_SURFACE_STATE_ASSOCIATED;

  wl_list_remove (&self->map.link);
  wl_list_remove (&self->unmap.link);
}


static void
handle_destroy (struct wl_listener *listener, void *data)
{
  PhocXWaylandUnmanaged *self = wl_container_of (listener, self, destroy);

  g_assert (PHOC_IS_XWAYLAND_UNMANAGED (self));
  g_assert (G_IS_OBJECT (self));

  g_object_unref (self);
}


static void
handle_set_override_redirect (struct wl_listener *listener, void *data)
{
  PhocXWaylandUnmanaged *self = wl_container_of (listener, self, set_override_redirect);
  struct wlr_xwayland_surface *wlr_xwayland_surface = self->wlr_xwayland_surface;
  PhocXWaylandSurfaceState state = PHOC_XWAYLAND_SURFACE_STATE_NONE;
  bool associated = wlr_xwayland_surface->surface != NULL;
  bool mapped = associated && wlr_xwayland_surface->surface->mapped;

  g_debug ("Switching unmanaged %p to new override-redirect", self);

  if (associated) {
    handle_dissociate (&self->dissociate, NULL);
    state |= PHOC_XWAYLAND_SURFACE_STATE_ASSOCIATED;
  }

  if (mapped) {
    handle_unmap (&self->unmap, NULL);
    state |= PHOC_XWAYLAND_SURFACE_STATE_MAPPED;
  }

  handle_destroy (&self->destroy, NULL);
  wlr_xwayland_surface->data = NULL;

  phoc_xwayland_unmanaged_new (wlr_xwayland_surface, state);
}

/* }}} */

static void
phoc_xwayland_unmanaged_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  PhocXWaylandUnmanaged *self = PHOC_XWAYLAND_UNMANAGED (object);

  switch (property_id) {
  case PROP_WLR_XWAYLAND_SURFACE:
    set_wlr_xwayland_surface (self, g_value_get_pointer (value));
    break;
  case PROP_SURFACE_STATE:
    self->state = g_value_get_flags (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_xwayland_unmanaged_constructed (GObject *object)
{
  PhocXWaylandUnmanaged *self = PHOC_XWAYLAND_UNMANAGED (object);
  struct wlr_xwayland_surface *wlr_xwayland_surface;

  g_assert (self->wlr_xwayland_surface);
  wlr_xwayland_surface = self->wlr_xwayland_surface;

  G_OBJECT_CLASS (phoc_xwayland_unmanaged_parent_class)->constructed (object);

  self->destroy.notify = handle_destroy;
  wl_signal_add (&wlr_xwayland_surface->events.destroy, &self->destroy);

  self->associate.notify = handle_associate;
  wl_signal_add (&wlr_xwayland_surface->events.associate, &self->associate);

  self->dissociate.notify = handle_dissociate;
  wl_signal_add (&wlr_xwayland_surface->events.dissociate, &self->dissociate);

  self->set_override_redirect.notify = handle_set_override_redirect;
  wl_signal_add (&wlr_xwayland_surface->events.set_override_redirect, &self->set_override_redirect);

  self->request_activate.notify = handle_request_activate;
  wl_signal_add (&wlr_xwayland_surface->events.request_activate, &self->request_activate);

  self->request_configure.notify = handle_request_configure;
  wl_signal_add (&wlr_xwayland_surface->events.request_configure, &self->request_configure);

  if (self->state & PHOC_XWAYLAND_SURFACE_STATE_ASSOCIATED)
    handle_associate (&self->associate, NULL);

  if (self->state & PHOC_XWAYLAND_SURFACE_STATE_MAPPED)
    handle_map (&self->map, wlr_xwayland_surface);

  g_debug ("Creating new unmanaged surface for %p (state: %d)",
           wlr_xwayland_surface,
           self->state);
}


static void
phoc_xwayland_unmanaged_finalize (GObject *object)
{
  PhocXWaylandUnmanaged *self = PHOC_XWAYLAND_UNMANAGED (object);

  wl_list_remove (&self->destroy.link);
  wl_list_remove (&self->associate.link);
  wl_list_remove (&self->dissociate.link);
  wl_list_remove (&self->request_activate.link);
  wl_list_remove (&self->request_configure.link);
  wl_list_remove (&self->set_override_redirect.link);

  self->wlr_xwayland_surface = NULL;

  G_OBJECT_CLASS (phoc_xwayland_unmanaged_parent_class)->finalize (object);
}


static void
phoc_xwayland_unmanaged_class_init (PhocXWaylandUnmanagedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = phoc_xwayland_unmanaged_set_property;
  object_class->constructed = phoc_xwayland_unmanaged_constructed;
  object_class->finalize = phoc_xwayland_unmanaged_finalize;

  /**
   * PhocXWaylandUnmanaged:wlr-xwayland-surface:
   *
   * The underlying wlroots xwayland-surface
   */
  props[PROP_WLR_XWAYLAND_SURFACE] =
    g_param_spec_pointer ("wlr-xwayland-surface", "", "",
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);
  /**
   * PhocXWaylandUnmanaged:surface-state:
   *
   * The surface state
   */
  props[PROP_SURFACE_STATE] =
    g_param_spec_flags ("surface-state", "", "",
                        PHOC_TYPE_XWAYLAND_SURFACE_STATE,
                        PHOC_XWAYLAND_SURFACE_STATE_NONE,
                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_xwayland_unmanaged_init (PhocXWaylandUnmanaged *self)
{
  wl_list_init (&self->map.link);
  wl_list_init (&self->unmap.link);
  wl_list_init (&self->set_geometry.link);
}


PhocXWaylandUnmanaged *
phoc_xwayland_unmanaged_new (struct wlr_xwayland_surface *surface,
                             PhocXWaylandSurfaceState     state)
{
  return g_object_new (PHOC_TYPE_XWAYLAND_UNMANAGED,
                       "wlr-xwayland-surface", surface,
                       "surface-state", state,
                       NULL);
}


void
phoc_xwayland_unmanaged_get_pos (PhocXWaylandUnmanaged *self, int *lx, int *ly)
{
  g_assert (PHOC_IS_XWAYLAND_UNMANAGED (self));
  g_assert (lx != NULL && ly != NULL);

  *lx = self->lx;
  *ly = self->ly;
}


struct wlr_surface *
phoc_xwayland_unmanaged_get_wlr_surface (PhocXWaylandUnmanaged *self)
{
  g_assert (PHOC_IS_XWAYLAND_UNMANAGED (self));

  return self->wlr_xwayland_surface->surface;
}


gboolean
phoc_xwayland_unmanaged_is_mapped (PhocXWaylandUnmanaged *self)
{
  g_assert (PHOC_IS_XWAYLAND_UNMANAGED (self));

  return !!(self->state & PHOC_XWAYLAND_SURFACE_STATE_MAPPED);
}
