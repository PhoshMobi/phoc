/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "workspace.h"

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * PhocWorkspaceMode:
 * @PHOC_WORKSPACE_MODE_FIXED: Fixed number of workspaces
 */
typedef enum _PhocWorkspaceMode {
  PHOC_WORKSPACE_MODE_FIXED = 0,
} PhocWorkspaceMode;

#define PHOC_TYPE_WORKSPACE_MANAGER (phoc_workspace_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhocWorkspaceManager, phoc_workspace_manager, PHOC, WORKSPACE_MANAGER,
                      GObject)

PhocWorkspaceManager *  phoc_workspace_manager_new              (void);
void                    phoc_workspace_manager_set_active       (PhocWorkspaceManager *self,
                                                                 PhocWorkspace        *workspace);
void                    phoc_workspace_manager_set_active_by_index (PhocWorkspaceManager *self,
                                                                    guint                 index);
PhocWorkspace *         phoc_workspace_manager_get_by_index     (PhocWorkspaceManager *self,
                                                                 guint                 index);
PhocWorkspace *         phoc_workspace_manager_get_active       (PhocWorkspaceManager *self);
guint                   phoc_workspace_manager_get_active_index (PhocWorkspaceManager *self);
guint                   phoc_workspace_manager_get_n_workspaces (PhocWorkspaceManager *self);

G_END_DECLS
