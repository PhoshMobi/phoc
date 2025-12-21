/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-toplevel-pidfd"

#include "phoc-config.h"

#include "server.h"
#include "toplevel-pidfd.h"
#include "phoc-foreign-toplevel-pidfd-v1-protocol.h"

#include <glib/gstdio.h>

#ifdef PHOC_HAVE_PIDFD
# include <sys/pidfd.h>
#endif

#define PHOC_FOREIGN_TOPLEVEL_PIDFD_V1_VERSION 1

/**
 * PhocToplevelPidfd:
 *
 * Send process information via the xx-foreign-toplevel-pidfd-v1 protocol
 */

struct _PhocToplevelPidfdManager {
  GObject           parent;

  struct wl_global *global;
  struct wl_list    resources; // wl_resource_get_link()

  GSList           *pidfds;
};
G_DEFINE_TYPE (PhocToplevelPidfdManager, phoc_toplevel_pidfd_manager, G_TYPE_OBJECT)

typedef struct {
  struct wl_list            resources; // wl_resource_get_link()
  struct wlr_ext_foreign_toplevel_handle_v1 *toplevel_handle;
  PhocToplevelPidfdManager *manager;

  struct wl_listener        toplevel_handle_handle_destroy;
} PhocToplevelPidfd;


static const struct phoc_foreign_toplevel_pidfd_v1_interface toplevel_pidfd_impl;
static PhocToplevelPidfdManager *
                          phoc_toplevel_pidfd_manager_from_resource (struct wl_resource *resource);


static int32_t
get_pidfd (PhocView *view)
{
#ifdef PHOC_HAVE_PIDFD
  pid_t pid = phoc_view_get_pid (view);
  int32_t pidfd = -1;

  if (pid < 0)
    return -1;

  pidfd = pidfd_open (pid, 0);
  if (pidfd < 0) {
    g_warning_once ("Failed to create pidfd %s", g_strerror (errno));
    return -1;
  }

  return pidfd;
#else
  return -1;
#endif
}


static gboolean
send_pidfd_iter (PhocDesktop *desktop, PhocView *view, gpointer user_data)
{
  PhocToplevelPidfd *toplevel_pidfd = user_data;
  struct wl_resource *resource;
  struct wlr_ext_foreign_toplevel_handle_v1 *handle;
  g_autofd int32_t pidfd = -1;

  handle = phoc_view_get_ext_foreign_toplevel_handle (view);
  if (handle != toplevel_pidfd->toplevel_handle)
    return TRUE;

  pidfd = phoc_view_get_pidfd (view);
  if (pidfd < 0) {
    g_debug ("Using pid fallback for %s", phoc_view_get_app_id (view));
    pidfd = get_pidfd (view);
  }

  if (pidfd < 0) {
    g_warning_once ("Failed to get pidfd for '%s'", phoc_view_get_app_id (view));
    return FALSE;
  }

  wl_resource_for_each (resource, &toplevel_pidfd->resources)
    phoc_foreign_toplevel_pidfd_v1_send_pidfd (resource, pidfd);

  return FALSE;
}


static void
handle_toplevel_handle_destroy (struct wl_listener *listener,void *data)
{
  PhocToplevelPidfd *pidfd = wl_container_of (listener, pidfd, toplevel_handle_handle_destroy);

  wl_list_remove (&pidfd->toplevel_handle_handle_destroy.link);
  wl_list_init (&pidfd->toplevel_handle_handle_destroy.link);

  pidfd->toplevel_handle = NULL;
}


static PhocToplevelPidfd *
phoc_toplevel_pidfd_new (struct wl_resource       *resource,
                         struct wl_resource       *handle_resource,
                         PhocToplevelPidfdManager *manager)
{
  PhocToplevelPidfd *pidfd = g_new0 (PhocToplevelPidfd, 1);

  pidfd->manager = manager;
  pidfd->toplevel_handle = wlr_ext_foreign_toplevel_handle_v1_from_resource (handle_resource);

  pidfd->toplevel_handle_handle_destroy.notify = handle_toplevel_handle_destroy;
  wl_signal_add (&pidfd->toplevel_handle->events.destroy, &pidfd->toplevel_handle_handle_destroy);

  wl_list_init (&pidfd->resources);

  return pidfd;
}


static void
phoc_toplevel_pidfd_destroy (PhocToplevelPidfd *pidfd)
{
  wl_list_remove (&pidfd->toplevel_handle_handle_destroy.link);
  wl_list_init (&pidfd->toplevel_handle_handle_destroy.link);

  pidfd->manager->pidfds = g_slist_remove (pidfd->manager->pidfds, pidfd);
  pidfd->manager = NULL;
  pidfd->toplevel_handle = NULL;
}


static void
phoc_toplevel_handle_pidfd_destroy (struct wl_client *client, struct wl_resource *resource)
{
  g_assert (wl_resource_instance_of (resource,
                                     &phoc_foreign_toplevel_pidfd_v1_interface,
                                     &toplevel_pidfd_impl));

  wl_resource_destroy (resource);
}


static const struct phoc_foreign_toplevel_pidfd_v1_interface toplevel_pidfd_impl = {
  .destroy = phoc_toplevel_handle_pidfd_destroy,
};


static void
phoc_toplevel_pidfd_resource_destroy (struct wl_resource *resource)
{
  PhocToplevelPidfd *pidfd;

  g_assert (wl_resource_instance_of (resource, &phoc_foreign_toplevel_pidfd_v1_interface,
                                     &toplevel_pidfd_impl));
  pidfd = wl_resource_get_user_data (resource);

  wl_list_remove (wl_resource_get_link (resource));

  if (wl_list_empty (&pidfd->resources))
    phoc_toplevel_pidfd_destroy (pidfd);
}


static void
phoc_toplevel_pidfd_manager_handle_get_pidfd (struct wl_client   *client,
                                              struct wl_resource *pidfd_manager_resource,
                                              struct wl_resource *handle_resource,
                                              uint32_t            id)
{
  PhocToplevelPidfdManager *self;
  struct wl_resource *resource;
  uint32_t version;
  PhocToplevelPidfd *pidfd;

  self = phoc_toplevel_pidfd_manager_from_resource (pidfd_manager_resource);
  g_assert (PHOC_IS_TOPLEVEL_PIDFD_MANAGER (self));

  version = wl_resource_get_version (pidfd_manager_resource);
  resource = wl_resource_create (client,
                                 &phoc_foreign_toplevel_pidfd_v1_interface,
                                 version,
                                 id);
  if (!resource) {
    wl_client_post_no_memory (client);
    return;
  }

  pidfd = phoc_toplevel_pidfd_new (resource, handle_resource, self);

  wl_resource_set_implementation (resource, &toplevel_pidfd_impl,
                                  pidfd, phoc_toplevel_pidfd_resource_destroy);
  wl_list_insert (&pidfd->resources, wl_resource_get_link (resource));

  phoc_desktop_for_each_view (phoc_server_get_desktop (phoc_server_get_default ()),
                              send_pidfd_iter,
                              pidfd);
  /* We only send pidfd once, so we're done here */
  phoc_foreign_toplevel_pidfd_v1_send_finished (resource);
}


static void
phoc_toplevel_pidfd_manager_handle_destroy (struct wl_client   *client,
                                            struct wl_resource *resource)
{
  wl_resource_destroy (resource);
}


static const struct
phoc_foreign_toplevel_pidfd_manager_v1_interface phoc_toplevel_pidfd_manager_impl = {
  .destroy = phoc_toplevel_pidfd_manager_handle_destroy,
  .get_pidfd = phoc_toplevel_pidfd_manager_handle_get_pidfd,
};


static void
phoc_toplevel_pidfd_manager_resource_destroy (struct wl_resource *resource)
{
  PhocToplevelPidfdManager *self = phoc_toplevel_pidfd_manager_from_resource (resource);

  g_assert (PHOC_IS_TOPLEVEL_PIDFD_MANAGER (self));

  g_debug ("Destroying phoc_foreign_toplevel_pidfd_manager %p (res %p)", self, resource);
  wl_list_remove (wl_resource_get_link (resource));
}


static void
phoc_toplevel_pidfd_manager_bind (struct wl_client *client,
                                  void             *data,
                                  uint32_t          version,
                                  uint32_t          id)
{
  PhocToplevelPidfdManager *self = PHOC_TOPLEVEL_PIDFD_MANAGER (data);
  struct wl_resource *resource;

  g_assert (PHOC_IS_TOPLEVEL_PIDFD_MANAGER (self));
  resource  = wl_resource_create (client,
                                  &phoc_foreign_toplevel_pidfd_manager_v1_interface,
                                  version,
                                  id);
  if (!resource) {
    wl_client_post_no_memory (client);
    return;
  }

  wl_resource_set_implementation (resource,
                                  &phoc_toplevel_pidfd_manager_impl,
                                  self,
                                  phoc_toplevel_pidfd_manager_resource_destroy);

  wl_list_insert (&self->resources, wl_resource_get_link (resource));
}


static PhocToplevelPidfdManager *
phoc_toplevel_pidfd_manager_from_resource (struct wl_resource *resource)
{
  g_assert (wl_resource_instance_of (resource,
                                     &phoc_foreign_toplevel_pidfd_manager_v1_interface,
                                     &phoc_toplevel_pidfd_manager_impl));

  return wl_resource_get_user_data (resource);
}


static void
phoc_toplevel_pidfd_manager_finalize (GObject *object)
{
  PhocToplevelPidfdManager *self = PHOC_TOPLEVEL_PIDFD_MANAGER (object);
  struct wl_resource *resource, *tmp;

  wl_resource_for_each_safe (resource, tmp, &self->resources) {
    wl_resource_set_user_data (resource, NULL);
    wl_list_remove (wl_resource_get_link (resource));
    wl_list_init (wl_resource_get_link (resource));
  }

  wl_global_destroy (self->global);

  G_OBJECT_CLASS (phoc_toplevel_pidfd_manager_parent_class)->finalize (object);
}


static void
phoc_toplevel_pidfd_manager_class_init (PhocToplevelPidfdManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phoc_toplevel_pidfd_manager_finalize;
}


static void
phoc_toplevel_pidfd_manager_init (PhocToplevelPidfdManager *self)
{
  struct wl_display *wl_display = phoc_server_get_wl_display (phoc_server_get_default ());

  wl_list_init (&self->resources);

  self->global = wl_global_create (wl_display,
                                   &phoc_foreign_toplevel_pidfd_manager_v1_interface,
                                   PHOC_FOREIGN_TOPLEVEL_PIDFD_V1_VERSION,
                                   self,
                                   phoc_toplevel_pidfd_manager_bind);

}


PhocToplevelPidfdManager *
phoc_toplevel_pidfd_manager_new (void)
{
  return g_object_new (PHOC_TYPE_TOPLEVEL_PIDFD_MANAGER, NULL);
}


struct wl_global *
phoc_toplevel_pidfd_manager_get_global (PhocToplevelPidfdManager *self)
{
  g_assert (PHOC_IS_TOPLEVEL_PIDFD_MANAGER (self));

  return self->global;
}
