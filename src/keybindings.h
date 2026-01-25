/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <glib-object.h>
#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-xkb-info.h>

#include <wlr/types/wlr_keyboard.h>

G_BEGIN_DECLS

#define PHOC_TYPE_KEYBINDINGS (phoc_keybindings_get_type ())

G_DECLARE_FINAL_TYPE (PhocKeybindings, phoc_keybindings, PHOC, KEYBINDINGS, GObject);

PhocKeybindings *phoc_keybindings_new (void);

/**
 * PhocKeyCombo:
 *
 * A combination of modifiers and a key describing a keyboard shortcut
 */
typedef struct {
  guint32      modifiers;
  xkb_keysym_t keysym;
} PhocKeyCombo;

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhocKeyCombo, g_free)

/**
 * PhocKeybindingsContext:
 *
 * Additional context to use when parsing keybindings
 */

typedef struct {
  xkb_keysym_t above_tab_keysym;
} PhocKeybindingsContext;

#define PHOC_TYPE_KEYBINDINGS_CONTEXT phoc_keybindings_context_get_type ()
GType                   phoc_keybindings_context_get_type (void) G_GNUC_CONST;

PhocKeybindingsContext *phoc_keybindings_context_new (void);
PhocKeybindingsContext *phoc_keybindings_context_copy (PhocKeybindingsContext *context);
void                    phoc_keybindings_context_free (PhocKeybindingsContext *context);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhocKeybindingsContext, phoc_keybindings_context_free)

typedef struct _PhocSeat PhocSeat;
gboolean                phoc_keybindings_handle_pressed (PhocKeybindings *self,
                                                         guint32          modifiers,
                                                         xkb_keysym_t    *pressed_keysyms,
                                                         guint32          length,
                                                         PhocSeat        *seat);
void                    phoc_keybindings_set_context (PhocKeybindings        *self,
                                                      PhocKeybindingsContext *context);
PhocKeyCombo *          phoc_keybindings_parse_accelerator (const char             *accelerator,
                                                            PhocKeybindingsContext *context);
void                    phoc_keybindings_load_settings (PhocKeybindings *self);

G_END_DECLS
