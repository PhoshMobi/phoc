/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-style-manager"

#include "phoc-config.h"

#include "style-manager.h"

#include <gdesktop-enums.h>
#include <gio/gio.h>

#define IF_KEY_ACCENT_COLOR     "accent-color"
#define IF_SCHEMA_NAME          "org.gnome.desktop.interface"

/* Accent colors from gnome-shell src/st/st-theme-context.c */
#define ACCENT_COLOR_BLUE       PHOC_HEX_COLOR (0x35, 0x84, 0xe4)
#define ACCENT_COLOR_TEAL       PHOC_HEX_COLOR (0x21, 0x90, 0xa4)
#define ACCENT_COLOR_GREEN      PHOC_HEX_COLOR (0x3a, 0x94, 0x4a)
#define ACCENT_COLOR_YELLOW     PHOC_HEX_COLOR (0xc8, 0x88, 0x00)
#define ACCENT_COLOR_ORANGE     PHOC_HEX_COLOR (0xed, 0x5b, 0x00)
#define ACCENT_COLOR_RED        PHOC_HEX_COLOR (0xe6, 0x2d, 0x42)
#define ACCENT_COLOR_PINK       PHOC_HEX_COLOR (0xd5, 0x61, 0x99)
#define ACCENT_COLOR_PURPLE     PHOC_HEX_COLOR (0x91, 0x41, 0xac)
#define ACCENT_COLOR_SLATE      PHOC_HEX_COLOR (0x6f, 0x83, 0x96)

/**
 * PhocStyleManager:
 *
 * The style manager is responsible for providing style information.
 */

enum {
  PROP_0,
  PROP_ACCENT_COLOR,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];


struct _PhocStyleManager {
  GObject    parent;

  PhocColor  accent_color;

  GSettings *interface_settings;
};
G_DEFINE_TYPE (PhocStyleManager, phoc_style_manager, G_TYPE_OBJECT)


static void
on_accent_color_changed (PhocStyleManager *self)
{
  switch (g_settings_get_enum (self->interface_settings, IF_KEY_ACCENT_COLOR)) {
  case G_DESKTOP_ACCENT_COLOR_TEAL:
    self->accent_color = ACCENT_COLOR_TEAL;
    break;
  case G_DESKTOP_ACCENT_COLOR_GREEN:
    self->accent_color = ACCENT_COLOR_GREEN;
    break;
  case G_DESKTOP_ACCENT_COLOR_YELLOW:
    self->accent_color = ACCENT_COLOR_YELLOW;
    break;
  case G_DESKTOP_ACCENT_COLOR_ORANGE:
    self->accent_color = ACCENT_COLOR_ORANGE;
    break;
  case G_DESKTOP_ACCENT_COLOR_RED:
    self->accent_color = ACCENT_COLOR_RED;
    break;
  case G_DESKTOP_ACCENT_COLOR_PINK:
    self->accent_color = ACCENT_COLOR_PINK;
    break;
  case G_DESKTOP_ACCENT_COLOR_PURPLE:
    self->accent_color = ACCENT_COLOR_PURPLE;
    break;
  case G_DESKTOP_ACCENT_COLOR_SLATE:
    self->accent_color = ACCENT_COLOR_SLATE;
    break;
  case G_DESKTOP_ACCENT_COLOR_BLUE:
  default:
    self->accent_color = ACCENT_COLOR_BLUE;
  }

  g_debug ("Setting accent bg color to %.2f/%.2f/%.2f",
           self->accent_color.red,
           self->accent_color.green,
           self->accent_color.blue);


}


static void
phoc_style_manager_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhocStyleManager *self = PHOC_STYLE_MANAGER (object);

  switch (property_id) {
  case PROP_ACCENT_COLOR:
    g_value_set_boxed (value, &self->accent_color);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_style_manager_dispose (GObject *object)
{
  PhocStyleManager *self = PHOC_STYLE_MANAGER (object);

  g_clear_object (&self->interface_settings);

  G_OBJECT_CLASS (phoc_style_manager_parent_class)->dispose (object);
}


static void
phoc_style_manager_class_init (PhocStyleManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_style_manager_get_property;
  object_class->dispose = phoc_style_manager_dispose;

  props[PROP_ACCENT_COLOR] =
    g_param_spec_boxed ("accent-color", "", "",
                        PHOC_TYPE_COLOR,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}


static void
phoc_style_manager_init (PhocStyleManager *self)
{
  self->interface_settings = g_settings_new (IF_SCHEMA_NAME);

  g_signal_connect_swapped (self->interface_settings,
                            "changed::" IF_KEY_ACCENT_COLOR,
                            G_CALLBACK (on_accent_color_changed),
                            self);
  on_accent_color_changed (self);
}

/**
 * phoc_style_manager_get_default:
 *
 * Get the server singleton.
 *
 * Returns: (transfer none): The style manager singleton
 */
PhocStyleManager *
phoc_style_manager_get_default (void)
{
  static PhocStyleManager *instance;

  if (G_UNLIKELY (instance == NULL)) {
    g_debug ("Creating style manager");
    instance = g_object_new (PHOC_TYPE_STYLE_MANAGER, NULL);

    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }

  return instance;
}

PhocColor
phoc_style_manager_get_accent_color (PhocStyleManager *self)
{
  g_assert (PHOC_IS_STYLE_MANAGER (self));

  return self->accent_color;
}
