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

	// Set transform to matrix.
	inline void nvgSetTransform(NVGcontext *c, float *tfm) {
		nvgResetTransform(c);
		nvgTransform(c, tfm[0], tfm[1], tfm[2], tfm[3], tfm[4], tfm[5]);
	}
}

#endif
