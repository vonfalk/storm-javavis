#pragma once

// Only used in the Gtk+ backend.
#ifdef GUI_GTK

// These are not included when compiled from the C file.
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

// TODO: Which version should we use?
#define NANOVG_GL2
#include "Linux/nanovg/src/nanovg.h"
#include "Linux/nanovg/src/nanovg_gl.h"
#endif
