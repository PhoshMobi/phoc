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
  PROP_WORKSPACE,
  PROP_N_WORKSPACES,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhocWorkspaceManager {
  GObject            parent;

  PhocWorkspaceMode  mode;
  GPtrArray         *workspaces;
  PhocWorkspace     *active;
};
G_DEFINE_TYPE (PhocWorkspaceManager, phoc_workspace_manager, G_TYPE_OBJECT)


static void
phoc_workspace_manager_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  PhocWorkspaceManager *self = PHOC_WORKSPACE_MANAGER (object);

  switch (property_id) {
  case PROP_WORKSPACE:
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
  PhocWorkspaceManager *self = PHOC_WORKSPACE_MANAGER(object);

  g_clear_pointer (&self->workspaces, g_ptr_array_unref);

  G_OBJECT_CLASS (phoc_workspace_manager_parent_class)->finalize (object);
}


static void
phoc_workspace_manager_class_init (PhocWorkspaceManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_workspace_manager_get_property;
  //object_class->set_property = phoc_workspace_manager_set_property;
  object_class->finalize = phoc_workspace_manager_finalize;

  props[PROP_N_WORKSPACES] =
    g_param_spec_boolean ("n-workspaces", "", "",
                          1,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  props[PROP_WORKSPACE] =
    g_param_spec_object ("active", "", "",
                         PHOC_TYPE_WORKSPACE,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_workspace_manager_init (PhocWorkspaceManager *self)
{
  guint n_workspaces;
  g_autoptr (GSettings) settings = g_settings_new (PREFS_SCHEMA_ID);

  /* TODO: adjust at runtime */
  self->mode = PHOC_WORKSPACE_MODE_FIXED;

  n_workspaces = g_settings_get_int (settings, KEY_NUM_WORKSPACES);
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
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_WORKSPACE]);
}


void
phoc_workspace_manager_set_active_by_index (PhocWorkspaceManager *self, guint index)
{
  PhocWorkspace *workspace;

  g_assert (PHOC_IS_WORKSPACE_MANAGER (self));

  if (index >= self->workspaces->len)
    return;

  workspace = g_ptr_array_index (self->workspaces, index);

  if (self->active == workspace)
    return;

  g_debug ("Switching to workspace %u", index);
  self->active = workspace;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_WORKSPACE]);
}
