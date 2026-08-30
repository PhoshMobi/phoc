/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define _GNU_SOURCE

#include "ext-foreign-toplevel-list-v1-client-protocol.h"
#include "phoc-foreign-toplevel-pidfd-v1-client-protocol.h"

#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <stdbool.h>

#ifdef __linux__
# include <sys/ioctl.h>
# include <linux/pidfd.h>
#endif

struct ext_foreign_toplevel_list_v1 *ext_toplevel_list;
struct phoc_foreign_toplevel_pidfd_manager_v1 *xx_toplevel_pidfd_manager;

typedef struct {
  char   *title;
  char   *app_id;
  char   *id;
  bool    done;

  pid_t   pid;
  int32_t pidfd;

  struct ext_foreign_toplevel_handle_v1 *handle;
  struct phoc_foreign_toplevel_pidfd_v1 *pidfd_v1;
} Toplevel;


static pid_t
pidfd_get_pid (int32_t pidfd)
{
#if __linux__
  struct pidfd_info info;

  if (ioctl (pidfd, PIDFD_GET_INFO, &info) < 0) {
    g_warning ("Failed to get pid from pidfd: %d", pidfd);
    return -1;
  }

  return info.pid;
#else
  return -1;
#endif
}


static void
toplevel_destroy (Toplevel *toplevel)
{
  g_autoptr (GError) err = NULL;

  g_clear_pointer (&toplevel->handle, ext_foreign_toplevel_handle_v1_destroy);
  g_clear_pointer (&toplevel->pidfd_v1, phoc_foreign_toplevel_pidfd_v1_destroy);
  if (!g_clear_fd (&toplevel->pidfd, &err))
    g_warning ("Failed to close fd of %s: %s", toplevel->id, err->message);

  g_free (toplevel->title);
  g_free (toplevel->app_id);
  g_free (toplevel->id);

  g_free (toplevel);
}


static void
print_toplevel (Toplevel *toplevel)
{
  g_print ("%s: (title='%s', id='%s', pid='%d')\n",
           toplevel->app_id,
           toplevel->title,
           toplevel->id,
           toplevel->pid);
}


static void
handle_toplevel_handle_closed (void                                  *data,
                               struct ext_foreign_toplevel_handle_v1 *ext_foreign_toplevel_handle_v1)
{
  Toplevel *toplevel = data;

  g_print ("%s (pid='%d') closed\n", toplevel->id, toplevel->pid);

  toplevel_destroy (toplevel);
}


static void
handle_toplevel_handle_done (void                                  *data,
                             struct ext_foreign_toplevel_handle_v1 *ext_foreign_toplevel_handle_v1)
{
  Toplevel *toplevel = data;

  toplevel->done = true;

  if (toplevel->done && toplevel->pid > 0)
    print_toplevel (toplevel);
}


static void
handle_toplevel_handle_title (void                                  *data,
                              struct ext_foreign_toplevel_handle_v1 *ext_foreign_toplevel_handle_v1,
                              const char                            *title)
{
  Toplevel *toplevel = data;

  toplevel->title = g_strdup (title);
}


static void
handle_toplevel_handle_app_id (void                                  *data,
                               struct ext_foreign_toplevel_handle_v1 *ext_foreign_toplevel_handle_v1,
                               const char                            *app_id)
{
  Toplevel *toplevel = data;

  toplevel->app_id = g_strdup (app_id);
}


static void
handle_toplevel_handle_id (void                                  *data,
                           struct ext_foreign_toplevel_handle_v1 *ext_foreign_toplevel_handle_v1,
                           const char                            *identifier)
{
  Toplevel *toplevel = data;

  toplevel->id = g_strdup (identifier);
}


static const struct ext_foreign_toplevel_handle_v1_listener toplevel_handle_listener =
{
  .closed = handle_toplevel_handle_closed,
  .done = handle_toplevel_handle_done,
  .title = handle_toplevel_handle_title,
  .app_id = handle_toplevel_handle_app_id,
  .identifier = handle_toplevel_handle_id,
};


static void
handle_pidfd (void                                  *data,
              struct phoc_foreign_toplevel_pidfd_v1 *phoc_foreign_toplevel_pidfd_v1,
              int32_t                                pidfd)
{
  Toplevel *toplevel = data;

  toplevel->pidfd = pidfd;
  toplevel->pid = pidfd_get_pid (toplevel->pidfd);

  if (toplevel->done && toplevel->pid > 0)
    print_toplevel (toplevel);
}


static void
handle_pidfd_finished (void                                  *data,
                       struct phoc_foreign_toplevel_pidfd_v1 *phoc_foreign_toplevel_pidfd_v1)
{
  Toplevel *toplevel = data;

  g_clear_pointer (&toplevel->pidfd_v1, phoc_foreign_toplevel_pidfd_v1_destroy);
}


static const struct phoc_foreign_toplevel_pidfd_v1_listener toplevel_pidfd_listener =
{
  .pidfd = handle_pidfd,
  .finished = handle_pidfd_finished,
};


static void
handle_toplevel (void                                  *data,
                 struct ext_foreign_toplevel_list_v1   *ext_foreign_toplevel_list_v1,
                 struct ext_foreign_toplevel_handle_v1 *handle)
{
  Toplevel *toplevel = g_new0 (Toplevel, 1);

  toplevel->handle = handle;
  toplevel->pidfd = -1;

  ext_foreign_toplevel_handle_v1_add_listener (handle, &toplevel_handle_listener, toplevel);

  toplevel->pidfd_v1 =
    phoc_foreign_toplevel_pidfd_manager_v1_get_pidfd (xx_toplevel_pidfd_manager,
                                                          handle);
  phoc_foreign_toplevel_pidfd_v1_add_listener (toplevel->pidfd_v1,
                                                   &toplevel_pidfd_listener,
                                                   toplevel);
}


static void
handle_finished (void                                *data,
                 struct ext_foreign_toplevel_list_v1 *ext_foreign_toplevel_list_v1)
{
  g_clear_pointer (&ext_toplevel_list, ext_foreign_toplevel_list_v1_destroy);
}


static const struct ext_foreign_toplevel_list_v1_listener toplevel_list_listener = {
  .toplevel = handle_toplevel,
  .finished = handle_finished,
};


static void
handle_global (void *data, struct wl_registry *registry,
               uint32_t name, const char *interface, uint32_t version)
{
  g_debug ("Interface found: %s", interface);
  if (strcmp (interface, ext_foreign_toplevel_list_v1_interface.name) == 0) {
    ext_toplevel_list = wl_registry_bind (registry,
                                          name,
                                          &ext_foreign_toplevel_list_v1_interface,
                                          1);
  } else if (strcmp (interface, phoc_foreign_toplevel_pidfd_manager_v1_interface.name) == 0) {
    xx_toplevel_pidfd_manager =
      wl_registry_bind (registry,
                        name,
                        &phoc_foreign_toplevel_pidfd_manager_v1_interface,
                        1);
  }
}


static void
handle_global_remove (void *data, struct wl_registry *registry, uint32_t name)
{
  // TODO
}

static const struct wl_registry_listener registry_listener = {
  .global = handle_global,
  .global_remove = handle_global_remove,
};


int
main (int argc, char *argv[])
{
  g_autoptr (GOptionContext) opt_context = NULL;
  g_autoptr (GError) err = NULL;
  const GOptionEntry options [] = {
    { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
  };
  struct wl_registry *registry;
  struct wl_display *display;

  opt_context = g_option_context_new ("- list toplevels and pids");
  g_option_context_add_main_entries (opt_context, options, NULL);
  if (!g_option_context_parse (opt_context, &argc, &argv, &err)) {
    g_critical ("Failed to parse options: %s", err->message);
    return EXIT_FAILURE;
  }

  display = wl_display_connect (NULL);
  if (display == NULL) {
    g_critical ("failed to create display");
    return EXIT_FAILURE;
  }

  registry = wl_display_get_registry (display);
  wl_registry_add_listener (registry, &registry_listener, NULL);
  wl_display_roundtrip (display);
  wl_registry_destroy (registry);

  if (ext_toplevel_list == NULL) {
    g_critical ("Compositor doesn't support ext-foreign-toplevel-list-v1 protocol");
    return EXIT_FAILURE;
  }

  if (xx_toplevel_pidfd_manager == NULL) {
    g_critical ("Compositor doesn't support xx-linux-foreign-toplevel-pidfd-v1 protocol");
    return EXIT_FAILURE;
  }

  ext_foreign_toplevel_list_v1_add_listener (ext_toplevel_list,
                                             &toplevel_list_listener,
                                             NULL);
  g_debug ("Listing toplevels");

  while (wl_display_dispatch (display) != -1)
    ;

  wl_display_disconnect (display);

  return EXIT_SUCCESS;
}
