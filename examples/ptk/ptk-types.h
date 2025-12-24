/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>

#include <stdint.h>

G_BEGIN_DECLS

typedef struct _PtkRGBA {
  float r, g, b, a;
} PtkRGBA;


typedef struct _PtkBox {
  uint32_t x, y, width, height;
} PtkBox;


typedef enum {
  PTK_ERROR_UNKNOWN,
  PTK_ERROR_SHADER_COMPILE,
} PtkErrors;

GQuark ptk_error_quark (void);

G_END_DECLS
