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

#ifdef __cplusplus

/**
 * Extensions to NVG:
 */
extern "C" {

	// Flush the rendering queue.
	void nvgFlush(NVGcontext *ctx);

	// Set the transform.
	void nvgSetTransform(NVGcontext *ctx, float *tfm);

	// Set the size of an image.
	void nvgResizeImage(NVGcontext *ctx, int id, int w, int h);

}

namespace gui {

	inline NVGcolor nvg(Color c) {
		NVGcolor r;
		r.r = c.r;
		r.g = c.g;
		r.b = c.b;
		r.a = c.a;
		return r;
	}

	// Apply a transform matrix.
	inline void nvgTransform(NVGcontext *c, Transform *tfm) {
		nvgTransform(c, tfm->at(0, 0), tfm->at(1, 0), tfm->at(0, 1), tfm->at(1, 1), tfm->at(0, 3), tfm->at(1, 3));
	}

}

#endif
