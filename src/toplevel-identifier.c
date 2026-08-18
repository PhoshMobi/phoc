/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-toplevel-identifier"

#include "phoc-config.h"

#include "server.h"
#include "toplevel-identifier.h"
#include "phoc-foreign-toplevel-identifier-v1-protocol.h"

#define PHOC_FOREIGN_TOPLEVEL_IDENTIFIER_V1_VERSION 1

/**
 * PhocToplevelIdentifierManager:
 *
 * Map `zwlr_foreign_toplevel_handle_v1`s to the identifier of the matching
 * `ext_foreign_toplevel_handle_v1` via the phoc-foreign-toplevel-identifier-v1 protocol
 */

struct _PhocToplevelIdentifierManager {
  GObject           parent;

  struct wl_global *global;
  struct wl_list    resources; // wl_resource_get_link()
};
G_DEFINE_TYPE (PhocToplevelIdentifierManager, phoc_toplevel_identifier_manager, G_TYPE_OBJECT)

static const struct phoc_foreign_toplevel_identifier_v1_interface toplevel_identifier_impl;
static PhocToplevelIdentifierManager *
                    phoc_toplevel_identifier_manager_from_resource (struct wl_resource *resource);


static void
phoc_toplevel_handle_identifier_destroy (struct wl_client *client, struct wl_resource *resource)
{
  g_assert (wl_resource_instance_of (resource,
                                     &phoc_foreign_toplevel_identifier_v1_interface,
                                     &toplevel_identifier_impl));

  wl_resource_destroy (resource);
}


static const struct phoc_foreign_toplevel_identifier_v1_interface toplevel_identifier_impl = {
  .destroy = phoc_toplevel_handle_identifier_destroy,
};


static void
phoc_toplevel_identifier_manager_handle_get_identifier (struct wl_client   *client,
                                                        struct wl_resource *manager_resource,
                                                        struct wl_resource *handle_resource,
                                                        uint32_t            id)
{
  PhocToplevelIdentifierManager *self;
  struct wl_resource *resource;
  struct wlr_foreign_toplevel_handle_v1 *wlr_handle;
  struct wlr_ext_foreign_toplevel_handle_v1 *ext_handle = NULL;
  uint32_t version;

  self = phoc_toplevel_identifier_manager_from_resource (manager_resource);
  g_assert (PHOC_IS_TOPLEVEL_IDENTIFIER_MANAGER (self));

  version = wl_resource_get_version (manager_resource);
  resource = wl_resource_create (client,
                                 &phoc_foreign_toplevel_identifier_v1_interface,
                                 version,
                                 id);
  if (!resource) {
    wl_client_post_no_memory (client);
    return;
  }

  wl_resource_set_implementation (resource, &toplevel_identifier_impl, NULL, NULL);

  /* The handle's user data is unset when the toplevel is already gone */
  wlr_handle = wl_resource_get_user_data (handle_resource);
  if (wlr_handle && wlr_handle->data)
    ext_handle = phoc_view_get_ext_foreign_toplevel_handle (PHOC_VIEW (wlr_handle->data));

  if (ext_handle && ext_handle->identifier)
    phoc_foreign_toplevel_identifier_v1_send_id (resource, ext_handle->identifier);
  else
    g_debug ("Toplevel has no identifier to send");

  /* We only send the identifier once, so we're done here */
  phoc_foreign_toplevel_identifier_v1_send_finished (resource);
}


static void
phoc_toplevel_identifier_manager_handle_destroy (struct wl_client   *client,
                                                 struct wl_resource *resource)
{
  wl_resource_destroy (resource);
}


static const struct
phoc_foreign_toplevel_identifier_manager_v1_interface phoc_toplevel_identifier_manager_impl = {
  .destroy = phoc_toplevel_identifier_manager_handle_destroy,
  .get_id = phoc_toplevel_identifier_manager_handle_get_identifier,
};


static void
phoc_toplevel_identifier_manager_resource_destroy (struct wl_resource *resource)
{
  PhocToplevelIdentifierManager *self = phoc_toplevel_identifier_manager_from_resource (resource);

  g_assert (PHOC_IS_TOPLEVEL_IDENTIFIER_MANAGER (self));

  g_debug ("Destroying phoc_foreign_toplevel_identifier_manager %p (res %p)", self, resource);
  wl_list_remove (wl_resource_get_link (resource));
}


static void
phoc_toplevel_identifier_manager_bind (struct wl_client *client,
                                       void             *data,
                                       uint32_t          version,
                                       uint32_t          id)
{
  PhocToplevelIdentifierManager *self = PHOC_TOPLEVEL_IDENTIFIER_MANAGER (data);
  struct wl_resource *resource;

  g_assert (PHOC_IS_TOPLEVEL_IDENTIFIER_MANAGER (self));
  resource  = wl_resource_create (client,
                                  &phoc_foreign_toplevel_identifier_manager_v1_interface,
                                  version,
                                  id);
  if (!resource) {
    wl_client_post_no_memory (client);
    return;
  }

  wl_resource_set_implementation (resource,
                                  &phoc_toplevel_identifier_manager_impl,
                                  self,
                                  phoc_toplevel_identifier_manager_resource_destroy);

  wl_list_insert (&self->resources, wl_resource_get_link (resource));
}


static PhocToplevelIdentifierManager *
phoc_toplevel_identifier_manager_from_resource (struct wl_resource *resource)
{
  g_assert (wl_resource_instance_of (resource,
                                     &phoc_foreign_toplevel_identifier_manager_v1_interface,
                                     &phoc_toplevel_identifier_manager_impl));

  return wl_resource_get_user_data (resource);
}


static void
phoc_toplevel_identifier_manager_finalize (GObject *object)
{
  PhocToplevelIdentifierManager *self = PHOC_TOPLEVEL_IDENTIFIER_MANAGER (object);
  struct wl_resource *resource, *tmp;

  wl_resource_for_each_safe (resource, tmp, &self->resources) {
    wl_resource_set_user_data (resource, NULL);
    wl_list_remove (wl_resource_get_link (resource));
    wl_list_init (wl_resource_get_link (resource));
  }

  wl_global_destroy (self->global);

  G_OBJECT_CLASS (phoc_toplevel_identifier_manager_parent_class)->finalize (object);
}


static void
phoc_toplevel_identifier_manager_class_init (PhocToplevelIdentifierManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phoc_toplevel_identifier_manager_finalize;
}


static void
phoc_toplevel_identifier_manager_init (PhocToplevelIdentifierManager *self)
{
  struct wl_display *wl_display = phoc_server_get_wl_display (phoc_server_get_default ());

  wl_list_init (&self->resources);

  self->global = wl_global_create (wl_display,
                                   &phoc_foreign_toplevel_identifier_manager_v1_interface,
                                   PHOC_FOREIGN_TOPLEVEL_IDENTIFIER_V1_VERSION,
                                   self,
                                   phoc_toplevel_identifier_manager_bind);

}


PhocToplevelIdentifierManager *
phoc_toplevel_identifier_manager_new (void)
{
  return g_object_new (PHOC_TYPE_TOPLEVEL_IDENTIFIER_MANAGER, NULL);
}


struct wl_global *
phoc_toplevel_identifier_manager_get_global (PhocToplevelIdentifierManager *self)
{
  g_assert (PHOC_IS_TOPLEVEL_IDENTIFIER_MANAGER (self));

  return self->global;
}
