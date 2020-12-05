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
#include "include/core/SkColor.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"

GrGLFuncPtr egl_get(void* ctx, const char name[]);
GrGLFuncPtr glx_get(void* ctx, const char name[]);

namespace gui {

	/**
	 * Convert various of our data types to Skia data types.
	 */

	// Color.
	inline SkColor4f skia(Color c) {
		return {c.r, c.g, c.b, c.a};
	}

	// Point.
	inline SkPoint skia(Point p) {
		return {p.x, p.y};
	}

	// Rectangle.
	inline SkRect skia(Rect r) {
		return {r.p0.x, r.p0.y, r.p1.x, r.p1.y};
	}

}

#endif

