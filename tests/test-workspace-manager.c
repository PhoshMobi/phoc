/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "desktop.h"
#include "server.h"
#include "view.h"
#include "workspace-manager.h"

#include <gio/gio.h>

#define PREFS_SCHEMA_ID    "org.gnome.desktop.wm.preferences"
#define KEY_NUM_WORKSPACES "num-workspaces"
#define N_WORKSPACES       4

typedef struct {
  PhocServer *server;
  PhocWorkspaceManager *manager;
  GSettings  *settings;
  GPtrArray  *views;
} TestFixture;


static void
fixture_setup (TestFixture *fixture, gconstpointer unused)
{
  PhocConfig *config = phoc_config_new_from_file (TEST_PHOC_INI);

  /* The workspace manager reads the number of workspaces on startup */
  fixture->settings = g_settings_new (PREFS_SCHEMA_ID);
  g_settings_set_int (fixture->settings, KEY_NUM_WORKSPACES, N_WORKSPACES);

  fixture->server = phoc_server_get_default ();
  phoc_server_set_debug_flags (fixture->server, PHOC_SERVER_DEBUG_FLAG_DISABLE_ANIMATIONS);
  g_assert_true (phoc_server_setup (fixture->server, config, NULL, NULL, PHOC_SERVER_FLAG_NONE));

  fixture->manager = phoc_desktop_get_workspace_manager (phoc_server_get_desktop (fixture->server));
  fixture->views = g_ptr_array_new_with_free_func (g_object_unref);

  g_assert_cmpuint (phoc_workspace_manager_get_n_workspaces (fixture->manager), ==, N_WORKSPACES);
  g_assert_cmpuint (phoc_workspace_manager_get_active_index (fixture->manager), ==, 0);
}


static void
fixture_teardown (TestFixture *fixture, gconstpointer unused)
{
  g_clear_object (&fixture->server);
  g_clear_pointer (&fixture->views, g_ptr_array_unref);
  g_clear_object (&fixture->settings);
  g_clear_object (&fixture->manager);
}


static PhocWorkspace *
get_workspace (TestFixture *fixture, guint index)
{
  return phoc_workspace_manager_get_by_index (fixture->manager, index);
}


static PhocView *
add_view (TestFixture *fixture, guint index, gboolean always_on_top)
{
  PhocView *view = g_object_new (PHOC_TYPE_VIEW, NULL);

  g_ptr_array_add (fixture->views, view);
  phoc_view_set_always_on_top (view, always_on_top);
  phoc_workspace_insert_view (get_workspace (fixture, index), view);

  return view;
}


static void
set_num_workspaces (TestFixture *fixture, guint n)
{
  g_settings_set_int (fixture->settings, KEY_NUM_WORKSPACES, n);
  g_assert_cmpuint (phoc_workspace_manager_get_n_workspaces (fixture->manager), ==, n);
}


static void
assert_views (PhocWorkspace *workspace, ...)
{
  GQueue *views = phoc_workspace_get_views (workspace);
  PhocView *view, *last = NULL;
  guint n = 0;
  va_list args;

  va_start (args, workspace);
  while ((view = va_arg (args, PhocView *))) {
    g_assert_true (phoc_workspace_get_view_by_index (workspace, n) == view);
    last = view;
    n++;
  }
  va_end (args);

  g_assert_cmpuint (g_queue_get_length (views), ==, n);
  g_assert_true (g_queue_peek_tail (views) == last);
}


static void
test_workspace_manager_grow (TestFixture *fixture, gconstpointer unused)
{
  PhocView *view = add_view (fixture, 0, FALSE);

  set_num_workspaces (fixture, 6);

  g_assert_cmpuint (phoc_workspace_manager_get_active_index (fixture->manager), ==, 0);
  assert_views (get_workspace (fixture, 0), view, NULL);
  for (guint i = 1; i < 6; i++)
    assert_views (get_workspace (fixture, i), NULL);
}


static void
test_workspace_manager_shrink_empty (TestFixture *fixture, gconstpointer unused)
{
  set_num_workspaces (fixture, 2);
  g_assert_cmpuint (phoc_workspace_manager_get_active_index (fixture->manager), ==, 0);

  set_num_workspaces (fixture, 1);
  g_assert_cmpuint (phoc_workspace_manager_get_active_index (fixture->manager), ==, 0);

  /* Workspaces can be added again afterwards */
  set_num_workspaces (fixture, 3);
  g_assert_cmpuint (phoc_workspace_manager_get_active_index (fixture->manager), ==, 0);
}


static void
test_workspace_manager_shrink_moves_views (TestFixture *fixture, gconstpointer unused)
{
  PhocView *a = add_view (fixture, 0, FALSE);
  PhocView *b = add_view (fixture, 1, FALSE);
  PhocView *c = add_view (fixture, 2, FALSE);
  PhocView *d = add_view (fixture, 3, FALSE);
  PhocWorkspace *last = get_workspace (fixture, 1);

  /* The last remaining workspace is the one on screen */
  phoc_workspace_manager_set_active (fixture->manager, last);
  set_num_workspaces (fixture, 2);

  g_assert_cmpuint (phoc_workspace_manager_get_active_index (fixture->manager), ==, 1);
  assert_views (get_workspace (fixture, 0), a, NULL);

  /* Views from the removed workspaces are below the ones already on screen */
  assert_views (last, b, c, d, NULL);

  /* The queue stays usable */
  add_view (fixture, 1, FALSE);
  g_assert_cmpuint (g_queue_get_length (phoc_workspace_get_views (last)), ==, 4);
}


static void
test_workspace_manager_shrink_active (TestFixture *fixture, gconstpointer unused)
{
  PhocView *b = add_view (fixture, 1, FALSE);
  PhocView *c = add_view (fixture, 2, FALSE);
  PhocView *d = add_view (fixture, 3, FALSE);
  PhocWorkspace *last;

  phoc_workspace_manager_set_active (fixture->manager, get_workspace (fixture, 3));
  set_num_workspaces (fixture, 2);

  /* The new last workspace is active now */
  last = get_workspace (fixture, 1);
  g_assert_cmpuint (phoc_workspace_manager_get_active_index (fixture->manager), ==, 1);
  g_assert_true (phoc_workspace_manager_get_active (fixture->manager) == last);

  /* Views from the formerly active workspace are on top, the others at the bottom */
  assert_views (last, d, b, c, NULL);
}


static void
test_workspace_manager_shrink_always_on_top (TestFixture *fixture, gconstpointer unused)
{
  PhocView *c = add_view (fixture, 2, FALSE);
  PhocView *on_top = add_view (fixture, 2, TRUE);
  PhocView *d = add_view (fixture, 3, FALSE);

  phoc_workspace_manager_set_active (fixture->manager, get_workspace (fixture, 3));
  set_num_workspaces (fixture, 3);

  /* Always on top views stay on top of the moved views */
  assert_views (get_workspace (fixture, 2), on_top, d, c, NULL);
}


static void
test_workspace_manager_shrink_hidden_always_on_top (TestFixture *fixture, gconstpointer unused)
{
  PhocView *b = add_view (fixture, 1, FALSE);
  PhocView *c = add_view (fixture, 2, FALSE);
  PhocView *on_top = add_view (fixture, 2, TRUE);

  phoc_workspace_manager_set_active (fixture->manager, get_workspace (fixture, 1));
  set_num_workspaces (fixture, 2);

  /* Always on top views are still above regular ones */
  assert_views (get_workspace (fixture, 1), on_top, b, c, NULL);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/phoc/workspace-manager/grow", TestFixture, NULL,
              fixture_setup, test_workspace_manager_grow, fixture_teardown);
  g_test_add ("/phoc/workspace-manager/shrink/empty", TestFixture, NULL,
              fixture_setup, test_workspace_manager_shrink_empty, fixture_teardown);
  g_test_add ("/phoc/workspace-manager/shrink/moves-views", TestFixture, NULL,
              fixture_setup, test_workspace_manager_shrink_moves_views, fixture_teardown);
  g_test_add ("/phoc/workspace-manager/shrink/active", TestFixture, NULL,
              fixture_setup, test_workspace_manager_shrink_active, fixture_teardown);
  g_test_add ("/phoc/workspace-manager/shrink/always-on-top", TestFixture, NULL,
              fixture_setup, test_workspace_manager_shrink_always_on_top, fixture_teardown);
  g_test_add ("/phoc/workspace-manager/shrink/hidden-always-on-top", TestFixture, NULL,
              fixture_setup, test_workspace_manager_shrink_hidden_always_on_top,
              fixture_teardown);

  return g_test_run ();
}
