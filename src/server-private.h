/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "server.h"

G_BEGIN_DECLS

void phoc_server_set_compatibles  (PhocServer *self, const char *const *compatibles);

G_END_DECLS
