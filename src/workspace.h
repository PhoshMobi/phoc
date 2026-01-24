/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "view.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_WORKSPACE (phoc_workspace_get_type ())

G_DECLARE_FINAL_TYPE (PhocWorkspace, phoc_workspace, PHOC, WORKSPACE, GObject)

/**
 * PhocWorkspaceViewIter:
 * @self: The worksapce
 * @view: The view
 * @user_data: The user data
 *
 * The iterator function that is invoked by the
 * `phoc_desktop_for_each_view`. The iterator can return `FALSE` if
 * iterating further views should be stopped.
 *
 * Returns: `TRUE` if the iteration should continue.
 */
typedef gboolean (*PhocWorkspaceViewIter)(PhocWorkspace *self, PhocView *view, gpointer user_data);

PhocWorkspace *         phoc_workspace_new                       (void);
void                    phoc_workspace_insert_view               (PhocWorkspace *self,
                                                                  PhocView      *view);
GQueue *                phoc_workspace_get_views                 (PhocWorkspace *self);
gboolean                phoc_workspace_has_view                  (PhocWorkspace *self,
                                                                  PhocView      *view);
gboolean                phoc_workspace_has_views                 (PhocWorkspace *self);
void                    phoc_workspace_move_view_to_top          (PhocWorkspace *self,
                                                                  PhocView      *view);
gboolean                phoc_workspace_remove_view               (PhocWorkspace *self,
                                                                  PhocView      *view);
PhocView *              phoc_workspace_get_view_by_index         (PhocWorkspace *self,
                                                                  guint          index);
void                    phoc_workspace_for_each_view             (PhocWorkspace *self,
                                                                  PhocWorkspaceViewIter view_iter,
                                                                  gpointer       user_data);
PhocView *              phoc_workspace_cycle                     (PhocWorkspace *self,
                                                                  gboolean forward);

G_END_DECLS
