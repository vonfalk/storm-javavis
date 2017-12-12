#include "stdafx.h"
#include "nanovg.h"
#ifdef GUI_GTK

// Disable some warnings in the library.
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wshift-negative-value"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

// Include the implementation.
#define NANOVG_GL_IMPLEMENTATION
#include "Linux/nanovg/src/nanovg.c"
#include "Linux/nanovg/src/nanovg_gl.h"
#endif
