/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-xx-cutouts"

#include "phoc-config.h"

#include "output-cutouts.h"
#include "server.h"
#include "xx-cutouts-v1.h"

#include "xx-cutouts-v1-protocol.h"

#include <glib-object.h>

#define XX_CUTOUTS_MANAGER_VERSION 1

/**
 * PhocXxCutoutsManager:
 *
 * The cutout manager informs toplevels about display cutouts in their
 * display area.
 *
 * TODO: Most of this will move to wlroots
 * https://gitlab.freedesktop.org/wlroots/wlroots/-/merge_requests/5217
 */

struct _PhocXxCutoutsManager {
  GObject parent;

  struct wl_global *global;
  GSList *resources;

  GSList *xx_cutouts;
  guint32 next_id;
};

G_DEFINE_TYPE (PhocXxCutoutsManager, phoc_xx_cutouts_manager, G_TYPE_OBJECT)

static PhocXxCutoutsManager *phoc_xx_cutouts_manager_from_resource (struct wl_resource *resource);
static PhocXxCutouts *phoc_xx_cutouts_from_resource         (struct wl_resource *resource);

static void
resource_handle_destroy (struct wl_client   *client,
                         struct wl_resource *resource)
{
  wl_resource_destroy (resource);
}


static void
handle_xx_cutouts_set_unhandled (struct wl_client   *client,
                                 struct wl_resource *resource,
                                 struct wl_array    *unhandled)
{
  PhocXxCutouts *xx_cutouts = wl_resource_get_user_data (resource);
  uint32_t *id;

  wl_array_for_each (id, unhandled)
    g_array_append_val (xx_cutouts->pending_unhandled, (*id));
}



static const struct xx_cutouts_v1_interface xx_cutouts_v1_impl = {
  .set_unhandled = handle_xx_cutouts_set_unhandled,
  .destroy = resource_handle_destroy,
};


static PhocXxCutouts *
phoc_xx_cutouts_from_resource (struct wl_resource *resource)
{
  g_assert (wl_resource_instance_of (resource, &xx_cutouts_v1_interface,
                                     &xx_cutouts_v1_impl));
  return wl_resource_get_user_data (resource);
}


static void
phoc_xx_cutouts_destroy (PhocXxCutouts *xx_cutouts)
{
  PhocXxCutoutsManager *xx_cutouts_manager;

  if (xx_cutouts == NULL)
    return;

  g_debug ("Destroying xx_cutouts %p (res %p)", xx_cutouts, xx_cutouts->resource);
  xx_cutouts_manager = PHOC_XX_CUTOUTS_MANAGER (xx_cutouts->xx_cutouts_manager);
  g_assert (PHOC_IS_XX_CUTOUTS_MANAGER (xx_cutouts_manager));

  if (xx_cutouts->view) {
    /* wlr signals */
    wl_list_remove (&xx_cutouts->xdg_toplevel_handle_destroy.link);
    wl_list_remove (&xx_cutouts->xdg_surface_handle_configure.link);
    wl_list_remove (&xx_cutouts->xdg_surface_handle_ack_configure.link);
  }
  xx_cutouts_manager->xx_cutouts = g_slist_remove (xx_cutouts_manager->xx_cutouts, xx_cutouts);

  xx_cutouts->view = NULL;
  wl_resource_set_user_data (xx_cutouts->resource, NULL);

  g_clear_pointer (&xx_cutouts->pending_unhandled, g_array_unref);
  g_clear_pointer (&xx_cutouts->current_unhandled, g_array_unref);

  g_assert (wl_list_empty (&xx_cutouts->events.unhandled_updated.listener_list));

  g_free (xx_cutouts);
}


static void
xx_cutouts_handle_resource_destroy (struct wl_resource *resource)
{
  PhocXxCutouts *xx_cutouts = phoc_xx_cutouts_from_resource (resource);

  phoc_xx_cutouts_destroy (xx_cutouts);
}


static void
xx_cutouts_handle_toplevel_destroy (struct wl_listener *listener, void *data)
{
  PhocXxCutouts *xx_cutouts = wl_container_of (listener,
                                               xx_cutouts,
                                               xdg_toplevel_handle_destroy);

  wl_list_remove (&xx_cutouts->xdg_toplevel_handle_destroy.link);
  wl_list_remove (&xx_cutouts->xdg_surface_handle_configure.link);
  wl_list_remove (&xx_cutouts->xdg_surface_handle_ack_configure.link);

  /* The view is unusable for us now */
  xx_cutouts->view = NULL;
}


static void
phoc_xx_cutouts_send_cutouts (PhocXxCutouts *xx_cutouts)
{
  PhocServer *server = phoc_server_get_default ();
  PhocDesktop *desktop = phoc_server_get_desktop (server);
  PhocOutput *output;
  PhocView *view;
  struct wlr_box box;

  view = xx_cutouts->view;

  /* If views are floating we don't send any cutout information */
  if (!phoc_view_is_maximized (view) && !phoc_view_is_fullscreen (view))
    goto out;

  box = phoc_view_get_pending_box (view);
  wl_list_for_each (output, &desktop->outputs, link) {
    pixman_region32_t overlaps;
    const pixman_box32_t *boxes;
    g_autoptr (GArray) corners = NULL;
    gboolean intersects;
    int n_overlaps;

    intersects = wlr_output_layout_intersects (desktop->layout, output->wlr_output, &box);
    if (!intersects)
      continue;

    pixman_region32_init (&overlaps);
    if (!phoc_output_get_cutout_boxes (output, view, &overlaps)) {
      pixman_region32_fini (&overlaps);
      continue;
    }

    boxes = pixman_region32_rectangles (&overlaps, &n_overlaps);
    for (int i = 0; i < n_overlaps; i++) {
      xx_cutouts_v1_send_cutout_box (xx_cutouts->resource,
                                     boxes[i].x1,
                                     boxes[i].y1,
                                     boxes[i].x2 - boxes[i].x1,
                                     boxes[i].y2 - boxes[i].y1,
                                     XX_CUTOUTS_V1_TYPE_CUTOUT,
                                     xx_cutouts->xx_cutouts_manager->next_id++);
    }
    pixman_region32_fini (&overlaps);

    corners = phoc_output_get_cutout_corners (output, view);
    if (!corners)
      continue;

    for (int i = 0; i < corners->len; i++) {
      PhocCutoutCorner corner = g_array_index (corners, PhocCutoutCorner, i);
      enum xx_cutouts_v1_corner_position pos;

      switch (corner.position) {
      case PHOC_CORNER_TOP_LEFT:
        pos = XX_CUTOUTS_V1_CORNER_POSITION_TOP_LEFT;
        break;
      case PHOC_CORNER_TOP_RIGHT:
        pos = XX_CUTOUTS_V1_CORNER_POSITION_TOP_RIGHT;
        break;
      case PHOC_CORNER_BOTTOM_RIGHT:
        pos = XX_CUTOUTS_V1_CORNER_POSITION_BOTTOM_RIGHT;
        break;
      case PHOC_CORNER_BOTTOM_LEFT:
        pos = XX_CUTOUTS_V1_CORNER_POSITION_BOTTOM_LEFT;
        break;
      default:
        g_assert_not_reached ();
      }

      xx_cutouts_v1_send_cutout_corner (xx_cutouts->resource,
                                        pos,
                                        corner.radius,
                                        xx_cutouts->xx_cutouts_manager->next_id++);
    }
  }

 out:
  xx_cutouts_v1_send_configure (xx_cutouts->resource);
}


static void
xx_cutouts_handle_xdg_surface_configure (struct wl_listener *listener, void *data)
{
  PhocXxCutouts *xx_cutouts;

  xx_cutouts = wl_container_of (listener, xx_cutouts, xdg_surface_handle_configure);
  phoc_xx_cutouts_send_cutouts (xx_cutouts);
}


static void
xx_cutouts_handle_xdg_surface_ack_configure (struct wl_listener *listener, void *data)
{
  PhocXxCutouts *xx_cutouts = wl_container_of (listener,
                                               xx_cutouts,
                                               xdg_surface_handle_ack_configure);

  g_clear_pointer (&xx_cutouts->current_unhandled, g_array_unref);
  xx_cutouts->current_unhandled = g_array_copy (xx_cutouts->pending_unhandled);
  g_array_remove_range (xx_cutouts->pending_unhandled, 0, xx_cutouts->pending_unhandled->len);

  wl_signal_emit_mutable (&xx_cutouts->events.unhandled_updated, xx_cutouts);
}


static void
handle_get_xx_cutouts (struct wl_client   *client,
                       struct wl_resource *xx_cutouts_manager_resource,
                       uint32_t            id,
                       struct wl_resource *wl_surface_resource)
{
  PhocXxCutoutsManager *self;
  g_autofree PhocXxCutouts *xx_cutouts = NULL;
  struct wlr_surface *wlr_surface;
  struct wlr_xdg_toplevel *wlr_xdg_toplevel;
  struct wlr_xdg_surface *wlr_xdg_surface;
  PhocView *view;
  int version;

  self = phoc_xx_cutouts_manager_from_resource (xx_cutouts_manager_resource);
  g_assert (PHOC_IS_XX_CUTOUTS_MANAGER (self));
  wlr_surface = wlr_surface_from_resource (wl_surface_resource);
  g_assert (wlr_surface);

  wlr_xdg_toplevel = wlr_xdg_toplevel_try_from_wlr_surface (wlr_surface);
  if (!wlr_xdg_toplevel) {
    wl_resource_post_error (xx_cutouts_manager_resource,
                            XX_CUTOUTS_MANAGER_V1_ERROR_INVALID_ROLE,
                            "Surface not a xdg-toplevel");
    return;
  }
  wlr_xdg_surface = wlr_xdg_surface_try_from_wlr_surface (wlr_surface);
  g_assert (wlr_xdg_surface);

  xx_cutouts = g_new0 (PhocXxCutouts, 1);

  version = wl_resource_get_version (xx_cutouts_manager_resource);
  xx_cutouts->xx_cutouts_manager = self;
  xx_cutouts->resource = wl_resource_create (client,
                                             &xx_cutouts_v1_interface,
                                             version,
                                             id);
  if (xx_cutouts->resource == NULL) {
    g_free (xx_cutouts);
    wl_client_post_no_memory (client);
    return;
  }

  g_debug ("New xx_cutouts %p (res %p)", xx_cutouts, xx_cutouts->resource);
  wl_resource_set_implementation (xx_cutouts->resource,
                                  &xx_cutouts_v1_impl,
                                  xx_cutouts,
                                  xx_cutouts_handle_resource_destroy);

  wl_signal_init (&xx_cutouts->events.unhandled_updated);
  xx_cutouts->pending_unhandled = g_array_new (FALSE, FALSE, sizeof(uint32_t));
  xx_cutouts->current_unhandled = g_array_new (FALSE, FALSE, sizeof(uint32_t));

  view = PHOC_VIEW (wlr_xdg_surface->data);
  g_assert (PHOC_IS_VIEW (view));
  xx_cutouts->view = view;

  xx_cutouts->xdg_toplevel_handle_destroy.notify = xx_cutouts_handle_toplevel_destroy;
  wl_signal_add (&wlr_xdg_toplevel->events.destroy, &xx_cutouts->xdg_toplevel_handle_destroy);

  xx_cutouts->xdg_surface_handle_configure.notify = xx_cutouts_handle_xdg_surface_configure;
  wl_signal_add (&wlr_xdg_surface->events.configure, &xx_cutouts->xdg_surface_handle_configure);

  xx_cutouts->xdg_surface_handle_ack_configure.notify =
    xx_cutouts_handle_xdg_surface_ack_configure;
  wl_signal_add (&wlr_xdg_surface->events.ack_configure,
                 &xx_cutouts->xdg_surface_handle_ack_configure);

  phoc_xx_cutouts_send_cutouts (xx_cutouts);

  self->xx_cutouts = g_slist_prepend (self->xx_cutouts, g_steal_pointer (&xx_cutouts));
}


static void
xx_cutouts_manager_handle_resource_destroy (struct wl_resource *resource)
{
  PhocXxCutoutsManager *self = wl_resource_get_user_data (resource);

  g_assert (PHOC_IS_XX_CUTOUTS_MANAGER (self));

  g_debug ("Destroying xx_cutouts_manager %p (res %p)", self, resource);
  self->resources = g_slist_remove (self->resources, resource);
}


static const struct xx_cutouts_manager_v1_interface xx_cutouts_manager_impl = {
  .destroy = resource_handle_destroy,
  .get_cutouts = handle_get_xx_cutouts,
};


static void
xx_cutouts_manager_bind (struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
  PhocXxCutoutsManager *self = PHOC_XX_CUTOUTS_MANAGER (data);
  struct wl_resource *resource  = wl_resource_create (client, &xx_cutouts_manager_v1_interface,
                                                      version, id);

  g_assert (PHOC_IS_XX_CUTOUTS_MANAGER (self));

  wl_resource_set_implementation (resource,
                                  &xx_cutouts_manager_impl,
                                  self,
                                  xx_cutouts_manager_handle_resource_destroy);

  self->resources = g_slist_prepend (self->resources, resource);
  return;
}


static PhocXxCutoutsManager *
phoc_xx_cutouts_manager_from_resource (struct wl_resource *resource)
{
  g_assert (wl_resource_instance_of (resource, &xx_cutouts_manager_v1_interface,
                                     &xx_cutouts_manager_impl));
  return wl_resource_get_user_data (resource);
}


static void
phoc_xx_cutouts_manager_finalize (GObject *object)
{
  PhocXxCutoutsManager *self = PHOC_XX_CUTOUTS_MANAGER (object);

  g_slist_free_full (self->xx_cutouts, (GDestroyNotify)phoc_xx_cutouts_destroy);

  wl_global_destroy (self->global);

  G_OBJECT_CLASS (phoc_xx_cutouts_manager_parent_class)->finalize (object);
}


static void
phoc_xx_cutouts_manager_class_init (PhocXxCutoutsManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phoc_xx_cutouts_manager_finalize;
}


static void
phoc_xx_cutouts_manager_init (PhocXxCutoutsManager *self)
{
  struct wl_display *wl_display = phoc_server_get_wl_display (phoc_server_get_default ());

  self->next_id = 1;

  self->global = wl_global_create (wl_display, &xx_cutouts_manager_v1_interface,
                                   XX_CUTOUTS_MANAGER_VERSION, self, xx_cutouts_manager_bind);

}


PhocXxCutoutsManager *
phoc_xx_cutouts_manager_new (void)
{
  return g_object_new (PHOC_TYPE_XX_CUTOUTS_MANAGER, NULL);
}

/**
 * phoc_xx_cutouts_manager_get_cutouts: (skip)
 * @self The cutouts manager
 *
 * Get the xdg_cutout_v1 objects.
 */
GSList *
phoc_xx_cutouts_manager_get_cutouts (PhocXxCutoutsManager *self)
{
  g_assert (PHOC_IS_XX_CUTOUTS_MANAGER (self));

  return self->xx_cutouts;
}
