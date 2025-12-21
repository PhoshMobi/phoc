/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ptk-demo"

#include "ptk.h"

#include "ptk-demo.h"

#include "cutout-rect-frag-src.h"
#include "cutout-rect-vert-src.h"

#include <GLES2/gl2.h>


#define LCARS_BLUEY     ((PtkRGBA){ 0.267, 0.333, 1 })
#define LCARS_RED       ((PtkRGBA){ 0.8, 0.267, 0.267 })
#define LCARS_ALMOND    ((PtkRGBA){ 1.0, 0.667, 0.565 })


typedef struct {
  uint32_t id;
  PtkBox   box;
} Cutout;


typedef struct {
  uint32_t id;
  uint32_t radius;
  enum xx_cutouts_v1_corner_position pos;
} Corner;


typedef struct _PtkDemoCutoutToplevel {
  PtkToplevel          *toplevel;
  struct wl_listener    clicked, resized, configured;
  struct xx_cutouts_v1 *xx_cutouts_v1;

  struct {
    GArray *corners;
    GArray *cutouts;
  } pending;

  struct {
    GArray *corners;
    GArray *cutouts;
  } current;

  PtkRGBA  color;
  PtkRGBA  cutout_color;
  gboolean needs_swap;

  struct {
    GLuint program;
    GLint  win;
    GLint  color;

    struct {
      GLint vertices;
      GLint pos;
      GLint size;
    } rect;

    struct {
      GLint vertices;
      GLint pos;
      GLint radius;
    } corner;
  } render;
} PtkDemoCutoutToplevel;


static GLuint
load_shader (GLenum type, const GLchar *shader_src, GError **error)
{
  GLuint shader;
  GLint compiled;

  shader = glCreateShader (type);
  if (shader == 0) {
    g_set_error_literal (error,
                         ptk_error_quark (),
                         PTK_ERROR_SHADER_COMPILE,
                         "Error creating shader");
    return 0;
  }

  glShaderSource (shader, 1, &shader_src, NULL);
  glCompileShader (shader);
  glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint info_len = 0;
    glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &info_len);
    if (info_len > 1) {
      g_autofree char *info_log = g_new0 (char, info_len);
      glGetShaderInfoLog (shader, info_len, NULL, info_log);
      g_set_error (error,
                   ptk_error_quark (),
                   PTK_ERROR_SHADER_COMPILE,
                   "Error compiling shader: %s", info_log);
    } else {
      g_set_error_literal (error,
                           ptk_error_quark (),
                           PTK_ERROR_SHADER_COMPILE,
                           "Error compiling shader");
    }
    glDeleteShader (shader);
    return 0;
  }
  return shader;
}


static bool
init (PtkDemoCutoutToplevel *self)
{
  g_autoptr (GError) err = NULL;
  GLuint vertex_shader;
  GLuint fragment_shader;
  GLuint program;
  GLint linked;

  /* Compile the shaders */
  vertex_shader = load_shader (GL_VERTEX_SHADER, cutout_rect_vert_src, &err);
  if (!vertex_shader) {
    g_critical ("Error loading vertex shader: %s", err->message);
    return false;
  }

  fragment_shader = load_shader (GL_FRAGMENT_SHADER, cutout_rect_frag_src, &err);
  if (!fragment_shader) {
    g_critical ("Error loading fragment shader: %s", err->message);
    return false;
  }

  /* Link the program */
  program = glCreateProgram ();
  g_assert (program);
  glAttachShader (program, vertex_shader);
  glAttachShader (program, fragment_shader);
  glLinkProgram (program);
  glGetProgramiv (program, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint info_len = 0;
    glGetProgramiv (program, GL_INFO_LOG_LENGTH, &info_len);
    if (info_len > 1) {
      g_autofree char *info_log = g_new0 (char, info_len);
      glGetProgramInfoLog (program, info_len, NULL, info_log);
      g_critical ("Error linking program:%s", info_log);
    }
    glDeleteProgram (program);
    return FALSE;
  }

  /* Get locations */
  self->render.program = program;
  self->render.color = glGetUniformLocation (program, "u_color");
  g_assert (self->render.color >= 0);
  self->render.rect.pos = glGetUniformLocation (program, "u_pos");
  g_assert (self->render.rect.pos >= 0);
  self->render.rect.size = glGetUniformLocation (program, "u_size");
  g_assert (self->render.rect.size >= 0);
  self->render.win = glGetUniformLocation (program, "u_win");
  g_assert (self->render.win >= 0);
  self->render.rect.vertices = glGetAttribLocation (program, "v_pos");
  g_assert (self->render.rect.vertices >= 0);

  glClearColor (self->color.r, self->color.g, self->color.b, 1.0);
  return TRUE;
}


static void
draw_rect (PtkDemoCutoutToplevel *self, const PtkBox *rect)
{
  PtkSurface *surface = PTK_SURFACE (self->toplevel);

  glUniform4f (self->render.color,
               self->cutout_color.r,
               self->cutout_color.g,
               self->cutout_color.b,
               1.0);

  glUniform2f (self->render.win, surface->width, surface->height);
  glUniform2f (self->render.rect.pos, rect->x, rect->y);
  glUniform2f (self->render.rect.size, rect->width, rect->height);
  glDrawArrays (GL_TRIANGLE_FAN, 0, 4);
}


static void
draw (PtkDemoCutoutToplevel *self)
{
  PtkSurface *surface = PTK_SURFACE (self->toplevel);
  GLfloat rect_vertices[] = {
    0.0f,  0.0f, 0.0f,
    0.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f,  0.0f, 0.0f
  };

  g_assert (surface->width && surface->height);

  glViewport (0, 0, surface->width, surface->height);
  glClear (GL_COLOR_BUFFER_BIT);
  glUseProgram (self->render.program);

  glVertexAttribPointer (self->render.rect.vertices, 3, GL_FLOAT, GL_FALSE, 0, rect_vertices);
  glEnableVertexAttribArray (self->render.rect.vertices);

  if (self->current.cutouts) {
    for (int i = 0; i < self->current.cutouts->len; i++) {
      Cutout cutout = g_array_index (self->current.cutouts, Cutout, i);
      draw_rect (self, &cutout.box);
    }
  }

  if (self->current.corners) {
    for (int i = 0; i < self->current.corners->len; i++) {
      Corner corner = g_array_index (self->current.corners, Corner, i);
      PtkBox box = { .width = corner.radius, .height = corner.radius };

      switch (corner.pos) {
      case XX_CUTOUTS_V1_CORNER_POSITION_TOP_LEFT:
        break;
      case XX_CUTOUTS_V1_CORNER_POSITION_TOP_RIGHT:
        box.x = surface->width - corner.radius;
        break;
      case XX_CUTOUTS_V1_CORNER_POSITION_BOTTOM_RIGHT:
        box.x = surface->width - corner.radius;
        box.y = surface->height - corner.radius;
        break;
      case XX_CUTOUTS_V1_CORNER_POSITION_BOTTOM_LEFT:
        box.y = surface->height - corner.radius;
        break;
      default:
        g_assert_not_reached ();
      }

      draw_rect (self, &box);
    }
  }

  self->needs_swap = true;
}


static void
handle_cutout_configure (void *data, struct xx_cutouts_v1 *xx_cutouts_v1)
{
  PtkDemoCutoutToplevel *self = data;

  g_clear_pointer (&self->current.corners, g_array_unref);
  self->current.corners = g_array_copy (self->pending.corners);
  g_array_remove_range (self->pending.corners, 0, self->pending.corners->len);

  g_clear_pointer (&self->current.cutouts, g_array_unref);
  self->current.cutouts = g_array_copy (self->pending.cutouts);
  g_array_remove_range (self->pending.cutouts, 0, self->pending.cutouts->len);

  for (int i = 0; i < self->current.cutouts->len; i++) {
    Cutout c = g_array_index (self->current.cutouts, Cutout, i);

    g_debug ("Cutout %d %u,%u %ux%u", c.id, c.box.x, c.box.y, c.box.width, c.box.height);
  }

  draw (self);
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
  PtkDemoCutoutToplevel *self = data;
  Cutout cutout;

  cutout.box.x = x;
  cutout.box.y = y;
  cutout.box.width = width;
  cutout.box.height = height;
  cutout.id = id;

  g_array_append_val (self->pending.cutouts, cutout);
}


static void
handle_cutout_corner (void                 *data,
                      struct xx_cutouts_v1 *xx_cutouts_v1,
                      uint32_t              position,
                      uint32_t              radius,
                      uint32_t              id)
{
  PtkDemoCutoutToplevel *self = data;
  Corner corner = { .radius = radius, .pos = position };

  g_array_append_val (self->pending.corners, corner);
}


const struct xx_cutouts_v1_listener xx_cutouts_v1_listener = {
  .cutout_box = handle_cutout_box,
  .cutout_corner = handle_cutout_corner,
  .configure = handle_cutout_configure,
};


static void
ptk_demo_toplevel_destroy (PtkDemoCutoutToplevel *self)
{
  g_clear_pointer (&self->pending.corners, g_array_unref);
  g_clear_pointer (&self->pending.cutouts, g_array_unref);

  g_clear_pointer (&self->current.corners, g_array_unref);
  g_clear_pointer (&self->current.cutouts, g_array_unref);

  wl_list_remove (&self->clicked.link);

  ptk_toplevel_destroy (self->toplevel);
  g_free (self);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PtkDemoCutoutToplevel, ptk_demo_toplevel_destroy)


static void
swap_buffers (PtkDemoCutoutToplevel *self)
{
  PtkSurface *surface = &self->toplevel->parent;

  if (!self->needs_swap)
    return;

  if (eglSwapBuffers (egl_display, surface->egl_surface) != EGL_TRUE)
    g_warning ("Swapping buffers failed");
  else
    self->needs_swap = false;
}


static void
handle_configured (struct wl_listener *listener, void *data)
{
  PtkDemoCutoutToplevel *self = wl_container_of (listener, self, configured);

  swap_buffers (self);
}


static void
handle_surface_resized (struct wl_listener *listener, void *data)
{
  PtkDemoCutoutToplevel *self = wl_container_of (listener, self, resized);

  draw (self);
}


static void
handle_surface_clicked (struct wl_listener *listener, void *data)
{
  PtkButtonEvent *event = data;
  PtkDemoCutoutToplevel *self = wl_container_of (listener, self, clicked);
  PtkToplevel *toplevel = self->toplevel;

  if (event->button == PTK_POINTER_EVENT_BUTTON_LEFT &&
      event->state == WL_POINTER_BUTTON_STATE_RELEASED) {

    if (self->toplevel->state & PTK_TOPLEVEL_STATE_MAXIMIZED)
      xdg_toplevel_unset_maximized (toplevel->xdg_toplevel);
    else if (self->toplevel->state & PTK_TOPLEVEL_STATE_FULLSCREEN)
      xdg_toplevel_unset_fullscreen (toplevel->xdg_toplevel);
    else
      xdg_toplevel_set_maximized (toplevel->xdg_toplevel);

  } else if (event->button == PTK_POINTER_EVENT_BUTTON_RIGHT &&
             event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
    PtkDisplay *display = ptk_display_get_default ();
    wl_display_disconnect (display->display);
  }
}


static PtkDemoCutoutToplevel *
ptk_demo_toplevel_new (PtkDisplay *display)
{
  PtkDemoCutoutToplevel *self = g_new0 (PtkDemoCutoutToplevel, 1);
  PtkSurface *surface;

  self->toplevel = ptk_toplevel_new ("Cutout example", 600, 300);
  self->color = LCARS_RED;
  self->cutout_color = LCARS_BLUEY;

  self->pending.corners = g_array_new (FALSE, FALSE, sizeof (Corner));
  self->pending.cutouts = g_array_new (FALSE, FALSE, sizeof (Cutout));

  self->clicked.notify = handle_surface_clicked;
  wl_signal_add (&PTK_SURFACE (self->toplevel)->events.clicked, &self->clicked);

  self->resized.notify = handle_surface_resized;
  wl_signal_add (&PTK_SURFACE (self->toplevel)->events.resized, &self->resized);

  self->configured.notify = handle_configured;
  wl_signal_add (&self->toplevel->events.configured, &self->configured);

  surface = &self->toplevel->parent;

  if (display->xx_cutouts_manager_v1) {
    self->xx_cutouts_v1 = xx_cutouts_manager_v1_get_cutouts (display->xx_cutouts_manager_v1,
                                                             surface->surface);
    xx_cutouts_v1_add_listener (self->xx_cutouts_v1, &xx_cutouts_v1_listener, self);
  } else {
    g_critical ("Compositor lacks cutout support");
  }

  return self;
}


bool
ptk_demo_cutouts (PtkDisplay *display)
{
  g_autoptr (PtkDemoCutoutToplevel) cutouts_toplevel = NULL;
  PtkSurface *surface;

  cutouts_toplevel = ptk_demo_toplevel_new (display);
  surface = PTK_SURFACE (cutouts_toplevel->toplevel);

  eglMakeCurrent (egl_display,
                  surface->egl_surface,
                  surface->egl_surface,
                  egl_context);

  if (!init (cutouts_toplevel))
    return false;

  draw (cutouts_toplevel);
  swap_buffers (cutouts_toplevel);

  while (wl_display_dispatch (display->display) != -1) {
    // This space intentionally left blank
  }

  return true;
}
