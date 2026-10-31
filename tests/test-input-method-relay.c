/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "testlib.h"


static gboolean
test_client_input_method_keyboard_grab (PhocTestClientGlobals *globals, gpointer data)
{
  struct zwp_input_method_v2 *input_method;
  struct zwp_input_method_keyboard_grab_v2 *grab;

  g_assert_nonnull (globals->seat);
  g_assert_nonnull (globals->input_method_manager);

  input_method = zwp_input_method_manager_v2_get_input_method (globals->input_method_manager,
                                                               globals->seat);
  g_assert_nonnull (input_method);
  g_assert_cmpint (wl_display_roundtrip (globals->display), >=, 0);

  grab = zwp_input_method_v2_grab_keyboard (input_method);
  g_assert_nonnull (grab);
  g_assert_cmpint (wl_display_roundtrip (globals->display), >=, 0);

  zwp_input_method_keyboard_grab_v2_release (grab);
  g_assert_cmpint (wl_display_roundtrip (globals->display), >=, 0);

  zwp_input_method_v2_destroy (input_method);
  g_assert_cmpint (wl_display_roundtrip (globals->display), >=, 0);

  return TRUE;
}


static void
test_input_method_keyboard_grab (void)
{
  PhocTestClientIface iface = {
    .client_run = test_client_input_method_keyboard_grab,
  };

  phoc_test_client_run (TEST_PHOC_CLIENT_TIMEOUT, &iface, NULL);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  PHOC_TEST_ADD ("/phoc/input-method-relay/keyboard-grab",
                 test_input_method_keyboard_grab);
  return g_test_run ();
}
