/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-focus-frame"

#include "phoc-config.h"

#include "focus-frame.h"
#include "style-manager.h"
#include "view.h"

/**
 * PhocFocusFrame:
 *
 * The colored frame drawn around a focused [class@View].
 */

enum {
  PROP_0,
  PROP_VIEW,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];

struct _PhocFocusFrame {
  PhocColorRect parent;

  PhocView     *view;
};
G_DEFINE_TYPE (PhocFocusFrame, phoc_focus_frame, PHOC_TYPE_COLOR_RECT)


static PhocBox
phoc_focus_frame_get_rect (PhocView *self)
{
  const int margin_width = PHOC_VIEW_WIN_MARGIN;
  PhocBox rect_box, geom_box;

  /* Grow the rect around the view a bit */
  phoc_view_get_box (self, &rect_box);
  phoc_view_get_geometry (self, &geom_box);
  rect_box.x -= margin_width - geom_box.x;
  rect_box.y -= margin_width - geom_box.y;
  rect_box.width += 2 * margin_width;
  rect_box.height += 2 * margin_width;

  return rect_box;
}


static void
on_rect_changed (PhocFocusFrame *self, PhocView *view)
{
  PhocBox box;

  box = phoc_focus_frame_get_rect (view);
  phoc_color_rect_set_box (PHOC_COLOR_RECT (self), &box);
}


static void
on_state_changed (PhocFocusFrame *self, GParamSpec *pspec, PhocView *view)
{
  if (phoc_view_is_floating (view) || phoc_view_is_tiled (view))
    phoc_bling_map (PHOC_BLING (self));
  else
    phoc_bling_unmap (PHOC_BLING (self));
}


static void
on_alpha_changed (PhocFocusFrame *self, GParamSpec *pspec, PhocView *view)
{
  PhocColor color;

  color = phoc_color_rect_get_color (PHOC_COLOR_RECT (self));
  color.alpha = phoc_view_get_alpha (view);
  phoc_color_rect_set_color (PHOC_COLOR_RECT (self), &color);
}


static void
on_accent_color_changed (PhocStyleManager *style_manager, GParamSpec *pspec, PhocColorRect *rect)
{
  PhocColor color, accent_color;

  color = phoc_color_rect_get_color (rect);

  accent_color = phoc_style_manager_get_accent_color (style_manager);
  accent_color.alpha = color.alpha;

  phoc_color_rect_set_color (rect, &accent_color);
}


static void
set_view (PhocFocusFrame *self, PhocView *view)
{
  PhocStyleManager *style_manager = phoc_style_manager_get_default ();

  g_assert (view == NULL || PHOC_IS_VIEW (view));

  if (self->view == view)
    return;

  g_set_weak_pointer (&self->view, view);

  if (self->view == NULL)
    return;

  g_object_connect (self->view,
                    "swapped-object-signal::pos-changed", on_rect_changed, self,
                    "swapped-object-signal::size-changed", on_rect_changed, self,
                    "swapped-object-signal::notify::state", on_state_changed, self,
                    "swapped-object-signal::notify::fullscreen", on_state_changed, self,
                    "swapped-object-signal::notify::alpha", on_alpha_changed, self,
                    NULL);
  g_signal_connect_object (style_manager,
                           "notify::accent-color",
                           G_CALLBACK (on_accent_color_changed),
                           self,
                           G_CONNECT_DEFAULT);
  on_accent_color_changed (style_manager, NULL, PHOC_COLOR_RECT (self));
  on_rect_changed (self, view);
  on_alpha_changed (self, NULL, view);
  on_state_changed (self, NULL, view);
}


static void
phoc_focus_frame_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PhocFocusFrame *self = PHOC_FOCUS_FRAME (object);

  switch (property_id) {
  case PROP_VIEW:
    set_view (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_focus_frame_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PhocFocusFrame *self = PHOC_FOCUS_FRAME (object);

  switch (property_id) {
  case PROP_VIEW:
    g_value_set_object (value, self->view);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_focus_frame_finalize (GObject *object)
{
  PhocFocusFrame *self = PHOC_FOCUS_FRAME (object);

  set_view (self, NULL);

  G_OBJECT_CLASS (phoc_focus_frame_parent_class)->finalize (object);
}


static void
phoc_focus_frame_class_init (PhocFocusFrameClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_focus_frame_get_property;
  object_class->set_property = phoc_focus_frame_set_property;
  object_class->finalize = phoc_focus_frame_finalize;

  props[PROP_VIEW] =
    g_param_spec_object ("view", "", "",
                         PHOC_TYPE_VIEW,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}


static void
phoc_focus_frame_init (PhocFocusFrame *self)
{
}


PhocFocusFrame *
phoc_focus_frame_new (PhocView *view)
{
  return g_object_new (PHOC_TYPE_FOCUS_FRAME, "view", view, NULL);
}
