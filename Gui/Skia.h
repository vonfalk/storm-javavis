#pragma once

#ifdef GUI_GTK

	/**
	 * Skia includes. Ignored by Mymake.
	 */


#include "include/gpu/GrDirectContext.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "include/gpu/gl/GrGLAssembleInterface.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"

GrGLFuncPtr egl_get(void* ctx, const char name[]);
GrGLFuncPtr glx_get(void* ctx, const char name[]);

#endif

