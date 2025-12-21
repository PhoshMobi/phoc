/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib.h"

#include "phoc-types.h"
#include "output-cutouts.h"
#include "xx-cutouts-v1.h"

#include "xx-cutouts-v1-client-protocol.h"


typedef struct {
  uint32_t id;
  PhocBox  box;
} Cutout;


typedef struct {
  struct {
    GArray *corners;
    GArray *cutouts;
  } pending;

  struct {
    GArray *corners;
    GArray *cutouts;
  } current;

  gboolean configured;

  /* Client side */
  PhocTestXdgToplevelSurface *xs;
  PhocTestClientGlobals      *globals;
  struct xx_cutouts_v1       *xx_cutouts_v1;
  /* Server side */
  struct wl_listener          unhandled_updated;
  gboolean got_unhandled;
} CutoutsInfo;


static void
cutouts_info_free (CutoutsInfo *info)
{
  g_clear_pointer (&info->pending.corners, g_array_unref);
  g_clear_pointer (&info->pending.cutouts, g_array_unref);

  g_clear_pointer (&info->current.corners, g_array_unref);
  g_clear_pointer (&info->current.cutouts, g_array_unref);

  g_free (info);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CutoutsInfo, cutouts_info_free);


static CutoutsInfo *
cutouts_info_new (PhocTestClientGlobals *globals)
{
  CutoutsInfo *info = g_new0 (CutoutsInfo, 1);

  info->pending.corners = g_array_new (FALSE, FALSE, sizeof (PhocCutoutCorner));
  info->pending.cutouts = g_array_new (FALSE, FALSE, sizeof (Cutout));
  info->globals = globals;

  return info;
}


static void
handle_cutout_configure (void *data, struct xx_cutouts_v1 *xx_cutouts_v1)
{
  CutoutsInfo *info = data;

  g_clear_pointer (&info->current.corners, g_array_unref);
  info->current.corners = g_array_copy (info->pending.corners);
  g_array_remove_range (info->pending.corners, 0, info->pending.corners->len);

  g_clear_pointer (&info->current.cutouts, g_array_unref);
  info->current.cutouts = g_array_copy (info->pending.cutouts);
  g_array_remove_range (info->pending.cutouts, 0, info->pending.cutouts->len);

  info->configured = TRUE;
}


static void
handle_cutout_box (void                 *data,
                   struct xx_cutouts_v1 *xx_cutouts_v1,
                   int32_t               x,
                   int32_t               y,
                   int32_t               width,
                   int32_t               height,
                   uint32_t              type,
                   uint32_t              id)
{
  CutoutsInfo *info = data;
  Cutout cutout;

  cutout.box.x = x;
  cutout.box.y = y;
  cutout.box.width = width;
  cutout.box.height = height;
  cutout.id = id;

  g_array_append_val (info->pending.cutouts, cutout);
}


static void
handle_cutout_corner (void                 *data,
                      struct xx_cutouts_v1 *xx_cutouts_v1,
                      uint32_t              position,
                      uint32_t              radius,
                      uint32_t              id)
{
  CutoutsInfo *info = data;
  PhocCutoutCorner corner = { .radius = radius, .position = position };

  g_array_append_val (info->pending.corners, corner);
}


const struct xx_cutouts_v1_listener cutouts_listener = {
  .cutout_box = handle_cutout_box,
  .cutout_corner = handle_cutout_corner,
  .configure = handle_cutout_configure,
};


static gboolean
test_client_xx_cutouts_none_client_run (PhocTestClientGlobals *globals, gpointer data)
{
  struct xx_cutouts_v1 *cutouts;
  PhocTestXdgToplevelSurface *xs;
  g_autoptr (CutoutsInfo) info = cutouts_info_new (globals);

  xs = phoc_test_xdg_toplevel_new (globals, 0, 0, "xx-cutouts-none");
  g_assert_nonnull (xs);

  cutouts = xx_cutouts_manager_v1_get_cutouts (globals->cutouts_manager, xs->wl_surface);
  xx_cutouts_v1_add_listener (cutouts, &cutouts_listener, info);

  wl_display_dispatch (globals->display);

  g_assert_true (info->configured);
  g_assert_cmpint (info->current.corners->len, ==, 0);
  g_assert_cmpint (info->current.cutouts->len, ==, 0);

  xx_cutouts_v1_destroy (cutouts);

  phoc_test_xdg_toplevel_free (xs);
  wl_display_roundtrip (globals->display);

  return TRUE;
}


static void
test_xx_cutouts_none (void)
{
  PhocTestClientIface iface = {
    .client_run = test_client_xx_cutouts_none_client_run,
  };

  phoc_test_client_run (TEST_PHOC_CLIENT_TIMEOUT, &iface, NULL);
}


static void
ack_configure_unhandled_callback (PhocTestXdgToplevelSurface *xs, gpointer data)
{
  CutoutsInfo *info = data;

  for (int i = 0; i < info->current.cutouts->len; i++) {
    Cutout *cutout = &g_array_index (info->current.cutouts, Cutout, i);
    struct wl_array unhandled;
    uint32_t *p;

    wl_array_init (&unhandled);
    p = wl_array_add (&unhandled, sizeof (uint32_t));
    *p = cutout->id;

    xx_cutouts_v1_set_unhandled (info->xx_cutouts_v1, &unhandled);
    wl_array_release (&unhandled);
  }
}


static void
handle_unhandled_updated (struct wl_listener *listener, void *data)
{
  CutoutsInfo *info = wl_container_of (listener, info, unhandled_updated);
  PhocXxCutouts *xx_cutouts = data;

  info->got_unhandled = TRUE;
  g_assert_cmpint (xx_cutouts->current_unhandled->len, ==, 1);

  wl_list_remove (&info->unhandled_updated.link);
}


static void
handle_cutout_unhandled_configure (void *data, struct xx_cutouts_v1 *xx_cutouts_v1)
{
  CutoutsInfo *info = data;

  handle_cutout_configure (data, xx_cutouts_v1);

  /* Add a signal handler for unhandled cutouts server side */
  if (info->current.cutouts->len && !info->got_unhandled) {
    /* The compositor runs in a different thread bus since so we need to
     * be careful with data access. */
    PhocDesktop *desktop = phoc_server_get_desktop (phoc_server_get_default ());
    PhocXxCutoutsManager *cutouts_manager;
    GSList *server_cutouts;
    PhocXxCutouts *xx_cutouts;

    g_assert (PHOC_IS_DESKTOP (desktop));
    cutouts_manager = phoc_desktop_get_xx_cutouts_manager (desktop);
    g_assert (PHOC_IS_XX_CUTOUTS_MANAGER (cutouts_manager));

    server_cutouts = phoc_xx_cutouts_manager_get_cutouts (cutouts_manager);

    g_assert_cmpint (g_slist_length (server_cutouts), ==, 1);
    xx_cutouts = server_cutouts->data;

    info->unhandled_updated.notify = handle_unhandled_updated;
    wl_signal_add (&xx_cutouts->events.unhandled_updated, &info->unhandled_updated);
  }

  wl_display_dispatch (info->globals->display);
}


const struct xx_cutouts_v1_listener cutouts_unhandled_listener = {
  .cutout_box = handle_cutout_box,
  .cutout_corner = handle_cutout_corner,
  .configure = handle_cutout_unhandled_configure,
};


static gboolean
test_client_xx_cutouts_unhandled_client_run (PhocTestClientGlobals *globals, gpointer data)
{
  guint32 color = 0xFF00FF00;
  struct xx_cutouts_v1 *cutouts;
  PhocTestXdgToplevelSurface *xs;
  g_autoptr (CutoutsInfo) info = cutouts_info_new (globals);

  xs = phoc_test_xdg_toplevel_new (globals, 0, 0, "xx-cutouts-unhandled");
  g_assert_nonnull (xs);
  info->xs = xs;

  /* Set callback to reject cutouts as unhandled */
  phoc_test_xdg_toplevel_set_ack_configure_callback (info->xs,
                                                     ack_configure_unhandled_callback,
                                                     info,
                                                     NULL);

  cutouts = xx_cutouts_manager_v1_get_cutouts (globals->cutouts_manager, xs->wl_surface);
  g_assert_nonnull (cutouts);
  xx_cutouts_v1_add_listener (cutouts, &cutouts_unhandled_listener, info);
  info->xx_cutouts_v1 = cutouts;

  xdg_toplevel_set_maximized (xs->xdg_toplevel);
  wl_display_dispatch (globals->display);

  wl_surface_commit (xs->wl_surface);
  phoc_test_xdg_update_buffer (globals, xs, color);

  wl_display_roundtrip (globals->display);

  g_assert_true (info->configured);
  g_assert_cmpint (info->current.corners->len, ==, 4);
  g_assert_cmpint (info->current.cutouts->len, ==, 1);

  /* Did we see `unhandled` server side */
  g_assert_true (info->got_unhandled);

  xx_cutouts_v1_destroy (cutouts);

  phoc_test_xdg_toplevel_free (xs);
  wl_display_roundtrip (globals->display);

  return TRUE;
}


static gboolean
test_client_xx_cutouts_unhandled_server_prepare (PhocServer *server, gpointer data)
{
  return TRUE;
}


static void
test_xx_cutouts_unhandled (void)
{
  PhocTestClientIface iface = {
    .server_prepare = test_client_xx_cutouts_unhandled_server_prepare,
    .client_run = test_client_xx_cutouts_unhandled_client_run,
    .debug_flags    = PHOC_SERVER_DEBUG_FLAG_FAKE_BUILTIN,
    .compatibles    = (const char * const[]){ "oneplus,fajita", NULL },
    .output_config  = (PhocTestOutputConfig){
      .width = 1080,
      .height = 2340,
      .scale = 1,
      .transform = WL_OUTPUT_TRANSFORM_NORMAL,
    },
  };

  phoc_test_client_run (TEST_PHOC_CLIENT_TIMEOUT, &iface, NULL);
}


static gboolean
test_client_xx_cutouts_common_client_run (PhocTestClientGlobals *globals, gpointer data)
{
  struct xx_cutouts_v1 *cutouts;
  PhocTestXdgToplevelSurface *xs;
  guint32 color = 0xFF00FF00;
  CutoutsInfo *info = data;

  xs = phoc_test_xdg_toplevel_new_with_buffer (globals, 0, 0, "xx-cutouts-common", color);
  g_assert_nonnull (xs);

  cutouts = xx_cutouts_manager_v1_get_cutouts (globals->cutouts_manager, xs->wl_surface);
  xx_cutouts_v1_add_listener (cutouts, &cutouts_listener, info);

  xdg_toplevel_set_maximized (xs->xdg_toplevel);
  wl_display_dispatch (globals->display);

  wl_surface_commit (xs->wl_surface);
  phoc_test_xdg_update_buffer (globals, xs, color);

  xx_cutouts_v1_destroy (cutouts);

  phoc_test_xdg_toplevel_free (xs);
  wl_display_roundtrip (globals->display);

  return TRUE;
}


static gboolean
test_client_xx_cutouts_common_server_prepare (PhocServer *server, gpointer data)
{
  return TRUE;
}


static void
test_xx_cutouts_common (int scale, enum wl_output_transform transform, CutoutsInfo *result)
{
  PhocTestClientIface iface = {
    .server_prepare = test_client_xx_cutouts_common_server_prepare,
    .client_run     = test_client_xx_cutouts_common_client_run,
    .debug_flags    = PHOC_SERVER_DEBUG_FLAG_FAKE_BUILTIN,
    .compatibles    = (const char * const[]){ "oneplus,fajita", NULL },
    .output_config  = (PhocTestOutputConfig){
      .width = 1080,
      .height = 2340,
      .scale = scale,
      .transform = transform,
    },
  };

  phoc_test_client_run (TEST_PHOC_CLIENT_TIMEOUT, &iface, result);
}

/* Scale 1, transform normal */
static void
test_xx_cutouts_scale1 (void)
{
  g_autoptr (CutoutsInfo) result = cutouts_info_new (NULL);
  Cutout cutout;

  test_xx_cutouts_common (1, WL_OUTPUT_TRANSFORM_NORMAL, result);

  g_assert_true (result->configured);

  g_assert_cmpint (result->current.corners->len, ==, 4);
  for (int i = 0; i < PHOC_NUM_CORNERS; i++) {
    phoc_assert_cmpcorner (&g_array_index (result->current.corners, PhocCutoutCorner, i),
                           &((PhocCutoutCorner){ i, 120}));
  }

  g_assert_cmpint (result->current.cutouts->len, ==, 1);
  cutout = g_array_index (result->current.cutouts, Cutout, 0);
  phoc_assert_cmpbox (&cutout.box, &((PhocBox){ 355, 0, 368, 78 }));
}

/* Scale 2, transform normal */
static void
test_xx_cutouts_scale2 (void)
{
  g_autoptr (CutoutsInfo) result = cutouts_info_new (NULL);
  Cutout cutout;

  test_xx_cutouts_common (2, WL_OUTPUT_TRANSFORM_NORMAL, result);

  g_assert_true (result->configured);

  g_assert_cmpint (result->current.corners->len, ==, 4);
  for (int i = 0; i < PHOC_NUM_CORNERS; i++) {
    phoc_assert_cmpcorner (&g_array_index (result->current.corners, PhocCutoutCorner, i),
                           &((PhocCutoutCorner){ i, 60}));
  }

  g_assert_cmpint (result->current.cutouts->len, ==, 1);
  cutout = g_array_index (result->current.cutouts, Cutout, 0);
  phoc_assert_cmpbox (&cutout.box, &((PhocBox){ 177, 0, 185, 39 }));
}

/* Scale 1, transform 90 */
static void
test_xx_cutouts_scale1_trans90 (void)
{
  g_autoptr (CutoutsInfo) result = cutouts_info_new (NULL);
  Cutout cutout;

  test_xx_cutouts_common (1, WL_OUTPUT_TRANSFORM_90, result);

  g_assert_true (result->configured);

  g_assert_cmpint (result->current.corners->len, ==, 4);
  for (int i = 0; i < PHOC_NUM_CORNERS; i++) {
    PhocCornerPosition pos = (PHOC_CORNER_TOP_LEFT + i) % PHOC_NUM_CORNERS;

    phoc_assert_cmpcorner (&g_array_index (result->current.corners, PhocCutoutCorner, i),
                           &((PhocCutoutCorner){ pos, 120}));
  }

  g_assert_cmpint (result->current.cutouts->len, ==, 1);
  cutout = g_array_index (result->current.cutouts, Cutout, 0);
  phoc_assert_cmpbox (&cutout.box, &((PhocBox){ 0, 357, 78, 368 }));
}

/* Scale 2, transform 90 */
static void
test_xx_cutouts_scale2_trans90 (void)
{
  g_autoptr (CutoutsInfo) result = cutouts_info_new (NULL);
  Cutout cutout;

  test_xx_cutouts_common (2, WL_OUTPUT_TRANSFORM_90, result);

  g_assert_true (result->configured);

  g_assert_cmpint (result->current.corners->len, ==, 4);
  for (int i = 0; i < PHOC_NUM_CORNERS; i++) {
    PhocCornerPosition pos = (PHOC_CORNER_TOP_LEFT + i) % PHOC_NUM_CORNERS;

    phoc_assert_cmpcorner (&g_array_index (result->current.corners, PhocCutoutCorner, i),
                           &((PhocCutoutCorner){ pos, 60}));
  }

  g_assert_cmpint (result->current.cutouts->len, ==, 1);
  cutout = g_array_index (result->current.cutouts, Cutout, 0);
  phoc_assert_cmpbox (&cutout.box, &((PhocBox){ 0, 178, 39, 185 }));
}


int
main (gint argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  PHOC_TEST_ADD ("/phoc/xx-cutouts/none", test_xx_cutouts_none);
  PHOC_TEST_ADD ("/phoc/xx-cutouts/unhandled", test_xx_cutouts_unhandled);
  PHOC_TEST_ADD ("/phoc/xx-cutouts/scale1", test_xx_cutouts_scale1);
  PHOC_TEST_ADD ("/phoc/xx-cutouts/scale2", test_xx_cutouts_scale2);
  PHOC_TEST_ADD ("/phoc/xx-cutouts/scale1-trans90", test_xx_cutouts_scale1_trans90);
  PHOC_TEST_ADD ("/phoc/xx-cutouts/scale2-trans90", test_xx_cutouts_scale2_trans90);

  return g_test_run ();
}
