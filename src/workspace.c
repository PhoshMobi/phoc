/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-workspace"

#include "phoc-config.h"

#include "workspace.h"

/**
 * PhocWorkspace:
 *
 * A workspace grouping a set of windows on an output
 */

struct _PhocWorkspace {
  GObject  parent;

  GQueue  *views;
};
G_DEFINE_TYPE (PhocWorkspace, phoc_workspace, G_TYPE_OBJECT)


static void
phoc_workspace_finalize (GObject *object)
{
  PhocWorkspace *self = PHOC_WORKSPACE (object);

  g_clear_pointer (&self->views, g_queue_free);

  G_OBJECT_CLASS (phoc_workspace_parent_class)->finalize (object);
}


static void
phoc_workspace_class_init (PhocWorkspaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phoc_workspace_finalize;
}


static void
phoc_workspace_init (PhocWorkspace *self)
{
  self->views = g_queue_new ();
}


PhocWorkspace *
phoc_workspace_new (void)
{
  return g_object_new (PHOC_TYPE_WORKSPACE, NULL);
}

/**
 * phoc_workspace_insert_view:
 * @self: the desktop
 * @view: the view to insert
 *
 * Insert the view into the queue of views. New views are inserted
 * at the front so they appear on top of other views.
 */
void
phoc_workspace_insert_view (PhocWorkspace *self, PhocView *view)
{
  g_assert (PHOC_IS_WORKSPACE (self));

  g_queue_push_head (self->views, view);
  phoc_workspace_move_view_to_top (self, view);
}

/**
 * phoc_workspace_move_view_to_top:
 * @self: the workspace
 * @view: a view
 *
 * Move the given view to the front of the view stack meaning that it
 * will be rendered on top of other views (but below always-on-top
 * views if `view` isn't a `always-on-top` view itself).
 */
void
phoc_workspace_move_view_to_top (PhocWorkspace *self, PhocView *view)
{
  GList *view_link;

  g_assert (PHOC_IS_WORKSPACE (self));

  view_link = g_queue_find (self->views, view);
  g_assert (view_link);

  g_queue_unlink (self->views, view_link);

  if (G_UNLIKELY (phoc_view_is_always_on_top (view))) {
    g_queue_push_head_link (self->views, view_link);
  } else {
    GList *l = NULL;

    for (l = self->views->head; l; l = l->next) {
      if (!phoc_view_is_always_on_top (PHOC_VIEW (l->data)))
        break;
    }

    g_queue_insert_before_link (self->views, l, view_link);
  }

  phoc_view_damage_whole (view);
}

/**
 * phoc_workspace_cycle:
 * @self: the workspace
 * @forward: Whether to cycle forward or backward through the views
 *
 * Cycles and re-arranges the current workspace views for focusing.
 * Depending on `forward` it cycles either forward or backward.
 *
 * Returns:(transfer none): The PhocView to be focused, or NULL.
 */
PhocView *
phoc_workspace_cycle (PhocWorkspace *self, gboolean forward)
{
  GList *link;
  PhocView *view;

  if (g_queue_get_length (self->views) < 2)
    return NULL;

  if (forward) {
    /* Move the last view first */
    link = g_queue_pop_tail_link (self->views);
    g_queue_push_head_link (self->views, link);
  } else {
    /* Move the first view to the end */
    link = g_queue_pop_head_link (self->views);
    g_queue_push_tail_link (self->views, link);
  }

  view = g_queue_peek_head (self->views);
  return view;
}

/**
 * phoc_workspace_get_views:
 * @self: the workspace
 *
 * Get the current views. Don't manipulate the queue directly. This is
 * only meant for reading.
 *
 * Returns:(transfer none): The views.
 */
GQueue *
phoc_workspace_get_views (PhocWorkspace *self)
{
  g_assert (PHOC_IS_WORKSPACE (self));

  return self->views;
}

/**
 * phoc_workspace_has_view:
 * @self: the workspace
 * @view: a view
 *
 * Checks if the given view is part of this workspace
 *
 * Returns: `TRUE` if the give view is on this workspace, otherwise `FALSE`
 */
gboolean
phoc_workspace_has_view (PhocWorkspace *self, PhocView *view)
{
  g_assert (PHOC_IS_WORKSPACE (self));

  return !!g_queue_find (self->views, view);
}

/**
 * phoc_workspace_has_views:
 * @self: the workspace
 *
 * Check whether the workspace has any views.
 *
 * Returns: %TRUE if there's at least on view, otherwise %FALSE
 */
gboolean
phoc_workspace_has_views (PhocWorkspace *self)
{
  g_assert (PHOC_IS_WORKSPACE (self));

  return !!self->views->head;
}

/**
 * phoc_workspace_remove_view:
 * @self: the workspace
 * @view: The view to remove
 *
 * Removes a view from the queue of views.
 *
 * Returns: %TRUE if the view was found, otherwise %FALSE
 */
gboolean
phoc_workspace_remove_view (PhocWorkspace *self, PhocView *view)
{
  g_assert (PHOC_IS_WORKSPACE (self));

  return g_queue_remove (self->views, view);
}

/**
 * phoc_workspace_get_view_by_index:
 * @self: the workspace
 * @index: the index to get the view for
 *
 * Gets the view at the given position in the queue. If the view is
 * not part of that workspace %NULL is returned.
 *
 * Returns:(transfer none)(nullable): the looked up view
 */
PhocView *
phoc_workspace_get_view_by_index (PhocWorkspace *self, guint index)
{
  g_assert (PHOC_IS_WORKSPACE (self));

  return g_queue_peek_nth (self->views, index);
}

/**
 * phoc_workspace_for_each_view:
 * @self: The workspace
 * @view_iter:(scope call): The iterator
 * @user_data: The user data
 *
 * Invokes `view_iter` on all views passing in `user_data`.
 */
void
phoc_workspace_for_each_view (PhocWorkspace         *self,
                              PhocWorkspaceViewIter  view_iter,
                              gpointer               user_data)
{
  g_assert (PHOC_IS_WORKSPACE (self));

  for (GList *l = self->views->head; l; l = l->next) {
    PhocView *view = PHOC_VIEW (l->data);
    gboolean cont;

    cont = (*view_iter)(self, view, user_data);
    if (!cont)
      return;
  }
}
