/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ptk-demo"

#include "ptk.h"
#include "ptk-demo.h"

#include <glib.h>

typedef struct {
  const char *name;
  const char *description;
  bool (*run) (PtkDisplay *display);
} Demo;


static const Demo demos[] = {
  { "cutouts", "Demo Display Cutouts", ptk_demo_cutouts },
  { "subsurfaces", "Demo Subsurfaces", ptk_demo_subsurface },
};


int
main (int argc, char **argv)
{
  g_autoptr (GOptionContext) opt_context = NULL;
  g_autoptr (GError) err = NULL;
  const char *example = NULL;
  gboolean list = FALSE;
  PtkDisplay *display;
  gint ret = 1;
  const GOptionEntry options [] = {
    {"run", 0, 0, G_OPTION_ARG_STRING, &example, "The example to run", NULL},
    {"list", 'l', 0, G_OPTION_ARG_NONE, &list, "List demos", NULL},
    { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
  };

  opt_context = g_option_context_new ("- Ptk demos");
  g_option_context_add_main_entries (opt_context, options, NULL);
  if (!g_option_context_parse (opt_context, &argc, &argv, &err)) {
    g_warning ("%s", err->message);
    g_clear_error (&err);
    return 1;
  }

  if (list) {
    for (int i = 0; i < G_N_ELEMENTS (demos); i++)
      g_print ("%s: %s\n", demos[i].name, demos[i].description);
    return 0;
  }

  display = ptk_display_init ();
  if (display == NULL) {
    g_critical ("Failed to create display");
    return 1;
  }

  if (!example) {
    g_message ("No example to run");
    return 1;
  }

  for (int i = 0; i < G_N_ELEMENTS (demos); i++) {
    if (g_str_equal (example, demos[i].name)) {
      ret = !(demos[i].run)(display);
      goto out;
    }
  }

  g_warning ("Unknown demo '%s'", example);
  ret = 1;

 out:
  ptk_display_uninit ();
  return ret;
}
