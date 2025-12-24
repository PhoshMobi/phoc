/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ptk-demo"


#include "ptk-demo.h"

#include <GLES2/gl2.h>


typedef struct _PtkDemoToplevel {
  PtkToplevel       *toplevel;
  struct wl_listener clicked;

  PtkRGBA color;
} PtkDemoToplevel;


typedef struct _PtkDemoSubsurface {
  PtkSubsurface     *subsurface;
  struct wl_listener prepare_frame, clicked;

  PtkRGBA color;
} PtkDemoSubsurface;


static void
ptk_demo_toplevel_destroy (PtkDemoToplevel *toplevel)
{
  wl_list_remove (&toplevel->clicked.link);

  ptk_toplevel_destroy (toplevel->toplevel);
  g_free (toplevel);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PtkDemoToplevel, ptk_demo_toplevel_destroy)

static PtkDemoSubsurface *ptk_demo_subsurface_new (PtkSurface *parent,
                                                   PtkSurface *root,
                                                   guint32     x,
                                                   guint32     y);

static void
ptk_demo_subsurface_destroy (PtkDemoSubsurface *subsurface)
{
  wl_list_remove (&subsurface->prepare_frame.link);
  wl_list_remove (&subsurface->clicked.link);

  ptk_subsurface_destroy (subsurface->subsurface);
  g_free (subsurface);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PtkDemoSubsurface, ptk_demo_subsurface_destroy)


static void
draw (PtkSurface *surface, PtkRGBA color)
{
  eglMakeCurrent (egl_display,
                  surface->egl_surface,
                  surface->egl_surface,
                  egl_context);

  glViewport (0, 0, surface->width, surface->height);
  glClearColor (color.r, color.g, color.b, color.a);
  glClear (GL_COLOR_BUFFER_BIT);

  if (eglSwapBuffers (egl_display, surface->egl_surface) != EGL_TRUE)
    g_warning ("Swapping buffers failed");
}


static void
handle_toplevel_clicked (struct wl_listener *listener, void *data)
{
  PtkButtonEvent *event = data;
  PtkDemoToplevel *demo = wl_container_of (listener, demo, clicked);
  PtkSurface *surface = PTK_SURFACE (demo->toplevel);
  PtkDemoSubsurface *demo_subsurface;

  if ((event->button == PTK_POINTER_EVENT_BUTTON_LEFT ||
       event->button == PTK_POINTER_EVENT_BUTTON_RIGHT)  &&
      event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
    gboolean above = event->button == PTK_POINTER_EVENT_BUTTON_LEFT;

    demo_subsurface = ptk_demo_subsurface_new (surface, surface, event->x, event->y);
    draw (PTK_SURFACE (demo_subsurface->subsurface), demo_subsurface->color);
    ptk_subsurface_place (demo_subsurface->subsurface, surface, above);
    ptk_subsurface_map (demo_subsurface->subsurface);
  }
}


static PtkDemoToplevel *
ptk_demo_toplevel_new (void)
{
  PtkDemoToplevel *demo = g_new0 (PtkDemoToplevel, 1);

  demo->toplevel = ptk_toplevel_new ("Subsurface example", 600, 300);
  demo->color = (PtkRGBA){ 1.0, 0.0, 0.0, 1.0 };

  demo->clicked.notify = handle_toplevel_clicked;
  wl_signal_add (&PTK_SURFACE (demo->toplevel)->events.clicked, &demo->clicked);

  return demo;
}


static void
handle_subsurface_clicked (struct wl_listener *listener, void *data)
{
  PtkButtonEvent *event = data;
  PtkDemoSubsurface *demo = wl_container_of (listener, demo, clicked);
  PtkSubsurface *parent = demo->subsurface;

  if (event->button == PTK_POINTER_EVENT_BUTTON_LEFT &&
      event->state == WL_POINTER_BUTTON_STATE_PRESSED) {
    PtkDemoSubsurface *new;

    new = ptk_demo_subsurface_new (PTK_SURFACE (parent), parent->root_surface, event->x, event->y);
    draw (PTK_SURFACE (new->subsurface), new->color);
    ptk_subsurface_map (new->subsurface);
  }
}


static void
handle_demo_subsurface_frame (struct wl_listener *listener, void *data)
{
  PtkDemoSubsurface *demo = wl_container_of (listener, demo, prepare_frame);

  demo->color.r += 0.01;
  if (demo->color.r > 1.0)
    demo->color.r = 0.0;

  draw (PTK_SURFACE (demo->subsurface), demo->color);
}


static PtkDemoSubsurface *
ptk_demo_subsurface_new (PtkSurface *parent, PtkSurface *main, guint32 x, guint32 y)
{
  PtkDemoSubsurface *demo = g_new0 (PtkDemoSubsurface, 1);

  demo->subsurface = ptk_subsurface_new (parent, main, x, y, 256, 256);
  demo->color = (PtkRGBA){ 0.0, 1.0, 0.0, 1.0 };

  demo->clicked.notify = handle_subsurface_clicked;
  wl_signal_add (&PTK_SURFACE (demo->subsurface)->events.clicked, &demo->clicked);

  demo->prepare_frame.notify = handle_demo_subsurface_frame;
  ptk_surface_add_frame_listener (PTK_SURFACE (demo->subsurface), &demo->prepare_frame);

  wl_subsurface_set_desync (demo->subsurface->subsurface);



  return demo;
}


bool
ptk_demo_subsurface (PtkDisplay *display)
{
  g_autoptr (PtkDemoToplevel) demo = NULL;

  demo = ptk_demo_toplevel_new ();
  draw (PTK_SURFACE (demo->toplevel), demo->color);

  while (wl_display_dispatch (display->display) != -1) {
    /* This space intentionally left blank */
  }

  return true;
}
