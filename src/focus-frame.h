/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "color-rect.h"
#include "view.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_FOCUS_FRAME (phoc_focus_frame_get_type ())

G_DECLARE_FINAL_TYPE (PhocFocusFrame, phoc_focus_frame, PHOC, FOCUS_FRAME, PhocColorRect)

PhocFocusFrame *phoc_focus_frame_new (PhocView * view);

G_END_DECLS
