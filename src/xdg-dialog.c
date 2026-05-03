/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-xdg-dialog"

#include "desktop.h"
#include "view-private.h"
#include "xdg-dialog.h"


typedef struct {
  struct wl_listener set_modal;
  struct wl_listener destroy;

  struct wlr_xdg_dialog_v1 *wlr_xdg_dialog;
} PhocXdgDialog;


static void
phoc_xdg_dialog_destroy (PhocXdgDialog *self)
{
  wl_list_remove (&self->set_modal.link);
  wl_list_remove (&self->destroy.link);

  g_free (self);
}


static void
update_modal (PhocXdgDialog *self)
{
  struct wlr_xdg_toplevel *wlr_xdg_toplevel = self->wlr_xdg_dialog->xdg_toplevel;
  PhocView *view = PHOC_VIEW (wlr_xdg_toplevel->base->data);
  gboolean modal;

  if (view == NULL)
    return;

  modal = self->wlr_xdg_dialog->modal;
  g_debug ("View %p is modal: %d", view, modal);
  phoc_view_set_modal (view, modal);
}


static void
handle_set_modal (struct wl_listener *listener, void *data)
{
  PhocXdgDialog *self = wl_container_of (listener, self, set_modal);

  update_modal (self);
}


static void
handle_destroy (struct wl_listener *listener, void *data)
{
  PhocXdgDialog *self = wl_container_of (listener, self, destroy);

  phoc_xdg_dialog_destroy (self);
}


static PhocXdgDialog *
phoc_xdg_dialog_create (struct wlr_xdg_dialog_v1 *wlr_xdg_dialog)
{
  PhocXdgDialog *self = g_new0 (PhocXdgDialog, 1);

  self->wlr_xdg_dialog = wlr_xdg_dialog;
  update_modal (self);

  self->set_modal.notify = handle_set_modal;
  wl_signal_add (&self->wlr_xdg_dialog->events.set_modal, &self->set_modal);

  self->destroy.notify = handle_destroy;
  wl_signal_add (&self->wlr_xdg_dialog->events.destroy, &self->destroy);

  return self;
}


void
phoc_handle_xdg_new_dialog (struct wl_listener *listener, void *data)
{
  struct wlr_xdg_dialog_v1 *wlr_xdg_dialog = data;

  g_debug ("new xdg dialog,  modal %d", wlr_xdg_dialog->modal);

  phoc_xdg_dialog_create (wlr_xdg_dialog);
}
