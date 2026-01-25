/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "keybindings.h"

#include <glib-object.h>


static void
test_keybindings_parse (void)
{
  g_autoptr (PhocKeybindingsContext) context = phoc_keybindings_context_new ();
  g_autoptr (PhocKeyCombo) combo = NULL;

  /* Simple case */
  combo = phoc_keybindings_parse_accelerator ("<shift>c", NULL);
  g_assert (combo);
  g_assert_cmpint (combo->modifiers, ==, WLR_MODIFIER_SHIFT);
  g_assert_cmpint (combo->keysym, ==, XKB_KEY_c);

  /* Using a capital letter doesn't affect the parsed keysym */
  g_free (combo);
  combo = phoc_keybindings_parse_accelerator ("<shift>C", NULL);
  g_assert (combo);
  g_assert_cmpint (combo->modifiers, ==, WLR_MODIFIER_SHIFT);
  g_assert_cmpint (combo->keysym, ==, XKB_KEY_c);

  /* Using a context doesn't affect the parsed keysym */
  g_free (combo);
  combo = phoc_keybindings_parse_accelerator ("<shift>c", context);
  g_assert (combo);
  g_assert_cmpint (combo->modifiers, ==, WLR_MODIFIER_SHIFT);
  g_assert_cmpint (combo->keysym, ==, XKB_KEY_c);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phoc/keybindings/parse", test_keybindings_parse);

  return g_test_run ();
}
