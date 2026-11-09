/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-workspace-manager"

#include "phoc-config.h"

#include "workspace-manager.h"
#include "seat.h"
#include "server.h"

#include <gio/gio.h>

#define KEY_NUM_WORKSPACES "num-workspaces"
#define PREFS_SCHEMA_ID    "org.gnome.desktop.wm.preferences"

/**
 * PhocWorkspaceManager:
 *
 * Manages the workspaces layout. Adds and removes new workspaces.
 */

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_N_WORKSPACES,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhocWorkspaceManager {
  GObject           parent;

  GSettings        *settings;
  PhocWorkspaceMode mode;
  GPtrArray        *workspaces;
  PhocWorkspace    *active;
};
G_DEFINE_TYPE (PhocWorkspaceManager, phoc_workspace_manager, G_TYPE_OBJECT)


static void
move_workspace_content (PhocWorkspace *from, PhocWorkspace *to, gboolean on_top)
{
  GQueue *views = phoc_workspace_get_views (from);
  GQueue *unmanaged = phoc_workspace_get_unmanaged (from);
  gpointer item;

  if (on_top) {
    while ((item = g_queue_peek_tail (views))) {
      phoc_workspace_remove_view (from, item);
      phoc_workspace_insert_view (to, item);
    }
  } else {
    while ((item = g_queue_peek_head (views))) {
      phoc_workspace_remove_view (from, item);
      phoc_workspace_append_view (to, item);
    }
  }

  /* TODO: Unmanaged surfaces will still be on top. We should close them. */
  while ((item = g_queue_peek_tail (unmanaged))) {
    phoc_workspace_remove_unmanaged (from, item);
    phoc_workspace_insert_unmanaged (to, item);
  }
}


static void
on_num_workspaces_setting_changed (PhocWorkspaceManager *self)
{
  guint n_old = self->workspaces->len;
  guint n_new;
  PhocWorkspace *last;

  if (self->mode != PHOC_WORKSPACE_MODE_FIXED)
    return;

  n_new = g_settings_get_int (self->settings, KEY_NUM_WORKSPACES);
  if (n_new == n_old)
    return;

  g_debug ("Number of workspaces changed from %u to %u", n_old, n_new);

  if (n_new > n_old) {
    for (guint i = n_old; i < n_new; i++)
      g_ptr_array_add (self->workspaces, phoc_workspace_new ());
    return;
  }

  /* Move everything from the workspaces that go away to the new last one */
  last = g_ptr_array_index (self->workspaces, n_new - 1);
  for (guint i = n_new; i < n_old; i++) {
    PhocWorkspace *workspace = g_ptr_array_index (self->workspaces, i);

    /* Views from hidden workspaces go to the bottom to not cover what's on screen */
    if (workspace != self->active)
      move_workspace_content (workspace, last, FALSE);
  }

  /* The active workspace goes last so its views end up on top */
  if (phoc_workspace_manager_get_active_index (self) >= n_new) {
    move_workspace_content (self->active, last, TRUE);
    phoc_workspace_manager_set_active (self, last);
  }

  g_ptr_array_remove_range (self->workspaces, n_new, n_old - n_new);
}


static void
phoc_workspace_manager_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  PhocWorkspaceManager *self = PHOC_WORKSPACE_MANAGER (object);

  switch (property_id) {
  case PROP_ACTIVE:
    g_value_set_object (value, phoc_workspace_manager_get_active (self));
    break;
  case PROP_N_WORKSPACES:
    g_value_set_uint (value, phoc_workspace_manager_get_n_workspaces (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_workspace_manager_finalize (GObject *object)
{
  PhocWorkspaceManager *self = PHOC_WORKSPACE_MANAGER (object);

  g_clear_object (&self->settings);
  g_clear_pointer (&self->workspaces, g_ptr_array_unref);

  G_OBJECT_CLASS (phoc_workspace_manager_parent_class)->finalize (object);
}


static void
phoc_workspace_manager_class_init (PhocWorkspaceManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_workspace_manager_get_property;
  object_class->finalize = phoc_workspace_manager_finalize;

  /**
   * PhocWorspaceManager::n-workspaces
   *
   * The number of workspaces
   */
  props[PROP_N_WORKSPACES] =
    g_param_spec_boolean ("n-workspaces", "", "",
                          1,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhocWorspaceManager::active
   *
   * The currently active workspace
   */
  props[PROP_ACTIVE] =
    g_param_spec_object ("active", "", "",
                         PHOC_TYPE_WORKSPACE,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_workspace_manager_init (PhocWorkspaceManager *self)
{
  guint n_workspaces;

  self->settings = g_settings_new (PREFS_SCHEMA_ID);
  self->mode = PHOC_WORKSPACE_MODE_FIXED;
  g_signal_connect_swapped (self->settings, "changed::" KEY_NUM_WORKSPACES,
                            G_CALLBACK (on_num_workspaces_setting_changed),
                            self);

  n_workspaces = g_settings_get_int (self->settings, KEY_NUM_WORKSPACES);
  self->workspaces = g_ptr_array_new_with_free_func (g_object_unref);

  for (int i = 0; i < n_workspaces; i++) {
    PhocWorkspace *workspace = phoc_workspace_new ();

    g_ptr_array_add (self->workspaces, workspace);
  }
  self->active = g_ptr_array_index (self->workspaces, 0);
}


PhocWorkspaceManager *
phoc_workspace_manager_new (void)
{
  return g_object_new (PHOC_TYPE_WORKSPACE_MANAGER, NULL);
}

guint
phoc_workspace_manager_get_n_workspaces (PhocWorkspaceManager *self)
{
  g_assert (PHOC_IS_WORKSPACE_MANAGER (self));

  return self->workspaces->len;
}

/**
 * phoc_workspace_manager_get_active:
 * @self: the workspace manager
 *
 * Get currently active workspace
 *
 * Returns:(transfer none): The active workspace.
 */
PhocWorkspace *
phoc_workspace_manager_get_active (PhocWorkspaceManager *self)
{
  g_assert (PHOC_IS_WORKSPACE_MANAGER (self));

  return self->active;
}


guint
phoc_workspace_manager_get_active_index (PhocWorkspaceManager *self)
{
  guint index = 0;
  gboolean success;
  g_assert (PHOC_IS_WORKSPACE_MANAGER (self));

  success = g_ptr_array_find (self->workspaces, self->active, &index);
  g_return_val_if_fail (success, 0);

  return index;
}

/**
 * phoc_workspace_manager_get_by_index:
 * @self: the workspace manager
 * @index: The index to get the workspace for
 *
 * Get the workspace at the given index.
 *
 * Returns:(transfer none): The workspace.
 */
PhocWorkspace *
phoc_workspace_manager_get_by_index (PhocWorkspaceManager *self, guint index)
{
  g_assert (PHOC_IS_WORKSPACE_MANAGER (self));

  g_return_val_if_fail (index < self->workspaces->len, NULL);

  return g_ptr_array_index (self->workspaces, index);
}


void
phoc_workspace_manager_set_active (PhocWorkspaceManager *self, PhocWorkspace *workspace)
{
  guint index;
  gboolean found;

  g_assert (PHOC_IS_WORKSPACE_MANAGER (self));

  if (self->active == workspace)
    return;

  found = g_ptr_array_find (self->workspaces, workspace, &index);
  g_assert (found);

  g_debug ("Switching to workspace %u", index);
  self->active = workspace;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ACTIVE]);
}

/**
 * phoc_workspace_manager_set_active_by_index:
 * @self: The workspace manager
 * @index: The index of the workspace to set active
 * @focus: Whether to focus the last active view on that workspace
 *
 * Activate the workspace with the given index. If `focus` is `TRUE`
 * the last focused view on that surface will be focused, otherwise
 * focus remains unchanged.
 */
void
phoc_workspace_manager_set_active_by_index (PhocWorkspaceManager *self,
                                            guint                 index,
                                            gboolean              focus)
{
  PhocWorkspace *workspace;
  PhocSeat *seat = phoc_server_get_last_active_seat (phoc_server_get_default ());

  g_assert (PHOC_IS_WORKSPACE_MANAGER (self));

  if (index >= self->workspaces->len)
    return;

  workspace = g_ptr_array_index (self->workspaces, index);

  if (self->active == workspace)
    return;

  g_debug ("Switching to workspace %u", index);
  self->active = workspace;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ACTIVE]);

  if (focus)
    phoc_seat_focus_workspace (seat, self->active);
}
