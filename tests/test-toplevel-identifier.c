/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "testlib.h"

typedef struct {
  char    *identifier;
  gboolean finished;
} IdentifierInfo;


static void
handle_id (void                                       *data,
           struct phoc_foreign_toplevel_identifier_v1 *id_v1,
           const char                                 *identifier)
{
  IdentifierInfo *info = data;

  g_assert_null (info->identifier);
  info->identifier = g_strdup (identifier);
}


static void
handle_finished (void                                       *data,
                 struct phoc_foreign_toplevel_identifier_v1 *id_v1)
{
  IdentifierInfo *info = data;

  info->finished = TRUE;
}


static const struct phoc_foreign_toplevel_identifier_v1_listener identifier_listener = {
  .id = handle_id,
  .finished = handle_finished,
};


static char *
get_identifier (PhocTestClientGlobals *globals, struct zwlr_foreign_toplevel_handle_v1 *handle)
{
  struct phoc_foreign_toplevel_identifier_v1 *id_v1;
  IdentifierInfo info = { 0 };

  id_v1 = phoc_foreign_toplevel_identifier_manager_v1_get_id (globals->toplevel_identifier_manager,
                                                              handle);
  g_assert_nonnull (id_v1);
  phoc_foreign_toplevel_identifier_v1_add_listener (id_v1, &identifier_listener, &info);
  g_assert_cmpint (wl_display_roundtrip (globals->display), !=, -1);
  g_assert_true (info.finished);

  phoc_foreign_toplevel_identifier_v1_destroy (id_v1);

  return info.identifier;
}


static gboolean
test_client_toplevel_identifier_simple_client_run (PhocTestClientGlobals *globals, gpointer data)
{
  PhocTestXdgToplevelSurface *xs_a, *xs_b;
  PhocTestExtForeignToplevel *ext_a, *ext_b;
  g_autofree char *id_a = NULL;
  g_autofree char *id_b = NULL;

  g_assert_nonnull (globals->toplevel_identifier_manager);

  xs_a = phoc_test_xdg_toplevel_new_with_buffer (globals,
                                                 0, 0,
                                                 "identifier-a",
                                                 PHOC_TEST_OPAQUE | PHOC_TEST_RED);
  g_assert_nonnull (xs_a);
  xs_b = phoc_test_xdg_toplevel_new_with_buffer (globals,
                                                 0, 0,
                                                 "identifier-b",
                                                 PHOC_TEST_OPAQUE | PHOC_TEST_GREEN);
  g_assert_nonnull (xs_b);

  ext_a = phoc_test_client_get_ext_foreign_toplevel_handle (globals, "identifier-a");
  g_assert_nonnull (ext_a);
  g_assert_nonnull (ext_a->identifier);
  ext_b = phoc_test_client_get_ext_foreign_toplevel_handle (globals, "identifier-b");
  g_assert_nonnull (ext_b);
  g_assert_nonnull (ext_b->identifier);
  g_assert_cmpstr (ext_a->identifier, !=, ext_b->identifier);

  id_a = get_identifier (globals, xs_a->foreign_toplevel->handle);
  g_assert_cmpstr (id_a, ==, ext_a->identifier);
  id_b = get_identifier (globals, xs_b->foreign_toplevel->handle);
  g_assert_cmpstr (id_b, ==, ext_b->identifier);

  phoc_test_xdg_toplevel_free (xs_a);
  phoc_test_xdg_toplevel_free (xs_b);
  g_assert_cmpint (wl_display_roundtrip (globals->display), !=, -1);

  return TRUE;
}


static void
test_toplevel_identifier_simple (void)
{
  PhocTestClientIface iface = {
    .client_run = test_client_toplevel_identifier_simple_client_run,
  };

  phoc_test_client_run (TEST_PHOC_CLIENT_TIMEOUT, &iface, NULL);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  PHOC_TEST_ADD ("/phoc/toplevel-identifier/simple", test_toplevel_identifier_simple);

  return g_test_run ();
}
