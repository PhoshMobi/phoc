/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "testlib.h"

#include <glib/gstdio.h>
#include <unistd.h>

typedef struct {
  int      pidfd;
  gboolean finished;
} PidfdInfo;


/* Get the pid a pidfd refers to or -1 */
static pid_t
pidfd_get_pid (int pidfd)
{
  g_autofree char *path = g_strdup_printf ("/proc/self/fdinfo/%d", pidfd);
  g_autofree char *contents = NULL;
  const char *line;
  int pid;

  if (!g_file_get_contents (path, &contents, NULL, NULL))
    return -1;

  line = strstr (contents, "Pid:");
  if (!line || sscanf (line, "Pid: %d", &pid) != 1)
    return -1;

  return pid;
}


static void
handle_pidfd (void                                  *data,
              struct phoc_foreign_toplevel_pidfd_v1 *pidfd_v1,
              int32_t                                pidfd)
{
  PidfdInfo *info = data;

  g_clear_fd (&info->pidfd, NULL);
  info->pidfd = pidfd;
}


static void
handle_finished (void                                  *data,
                 struct phoc_foreign_toplevel_pidfd_v1 *pidfd_v1)
{
  PidfdInfo *info = data;

  info->finished = TRUE;
}


static const struct phoc_foreign_toplevel_pidfd_v1_listener pidfd_listener = {
  .pidfd = handle_pidfd,
  .finished = handle_finished,
};


static gboolean
test_client_toplevel_pidfd_simple_client_run (PhocTestClientGlobals *globals, gpointer data)
{
  PhocTestXdgToplevelSurface *xs;
  PhocTestExtForeignToplevel *ext_toplevel;
  struct phoc_foreign_toplevel_pidfd_v1 *pidfd_v1;
  PidfdInfo info = { .pidfd = -1 };

  g_assert_nonnull (globals->toplevel_pidfd_manager);

  xs = phoc_test_xdg_toplevel_new_with_buffer (globals, 0, 0,
                                               "toplevel-pidfd",
                                               PHOC_TEST_OPAQUE | PHOC_TEST_GREEN);
  g_assert_nonnull (xs);

  ext_toplevel = phoc_test_client_get_ext_foreign_toplevel_handle (globals, "toplevel-pidfd");
  g_assert_nonnull (ext_toplevel);

  pidfd_v1 = phoc_foreign_toplevel_pidfd_manager_v1_get_pidfd (globals->toplevel_pidfd_manager,
                                                               ext_toplevel->handle);
  g_assert_nonnull (pidfd_v1);
  phoc_foreign_toplevel_pidfd_v1_add_listener (pidfd_v1, &pidfd_listener, &info);
  g_assert_cmpint (wl_display_roundtrip (globals->display), !=, -1);

  g_assert_true (info.finished);
  g_assert_cmpint (info.pidfd, >=, 0);
  /* Client and compositor share the same process */
  g_assert_cmpint (pidfd_get_pid (info.pidfd), ==, getpid ());
  g_clear_fd (&info.pidfd, NULL);

  phoc_foreign_toplevel_pidfd_v1_destroy (pidfd_v1);

  phoc_test_xdg_toplevel_free (xs);
  g_assert_cmpint (wl_display_roundtrip (globals->display), !=, -1);

  return TRUE;
}


static void
test_toplevel_pidfd_simple (void)
{
  PhocTestClientIface iface = {
    .client_run = test_client_toplevel_pidfd_simple_client_run,
  };

  phoc_test_client_run (TEST_PHOC_CLIENT_TIMEOUT, &iface, NULL);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  PHOC_TEST_ADD ("/phoc/toplevel-pidfd/simple", test_toplevel_pidfd_simple);

  return g_test_run ();
}
