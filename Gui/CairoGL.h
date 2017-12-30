#pragma once

#ifdef GUI_GTK

#include <cairo/cairo.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <EGL/egl.h>

/**
 * This file basically contains the definitions from cairo-gl.h. That file is not present
 * everywhere, so we cannot use that file.
 *
 * TODO: We may not always have access to GLX, for example when running on Wayland.
 */

extern "C" {

	/**
	 * Generic GL functionality.
	 */

	cairo_surface_t *cairo_gl_surface_create(cairo_device_t *device,
											cairo_content_t content, int width, int height);

	cairo_surface_t *cairo_gl_surface_create_for_texture(cairo_device_t *device,
														cairo_content_t content, unsigned int tex,
														int width, int height);

	void cairo_gl_surface_set_size(cairo_surface_t *device, int width, int height);

	int cairo_gl_surface_get_width(cairo_surface_t *surface);

	int cairo_gl_surface_get_height(cairo_surface_t *surface);

	void cairo_gl_surface_swapbuffers(cairo_surface_t *surface);

	void cairo_gl_device_set_thread_aware(cairo_device_t *device, cairo_bool_t thread_aware);

	/**
	 * GLX devices.
	 */

	cairo_device_t *cairo_glx_device_create(Display *dpy, GLXContext context);

	Display *cairo_glx_device_get_display(cairo_device_t *device);

	GLXContext cairo_glx_device_get_context(cairo_device_t *device);

	cairo_surface_t *cairo_gl_surface_create_for_window(cairo_device_t *device, Window win, int width, int height);

	/**
	 * EGL devices.
	 */

	cairo_device_t *cairo_egl_device_create(EGLDisplay dpy, EGLContext context);

	EGLDisplay cairo_egl_device_get_display(cairo_device_t *device);

	EGLSurface cairo_egl_device_get_context(cairo_device_t *device);

	cairo_surface_t *cairo_gl_surface_create_for_egl(cairo_device_t *device, EGLSurface egl, int width, int height);

}

#endif
