#pragma once

#include <stdbool.h>
#include <wayland-client.h>

#include <glib.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

G_BEGIN_DECLS

extern EGLDisplay egl_display;
extern EGLConfig egl_config;
extern EGLContext egl_context;

extern PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC eglCreatePlatformWindowSurfaceEXT;

bool egl_init (struct wl_display *display);

void egl_finish (void);

G_END_DECLS
