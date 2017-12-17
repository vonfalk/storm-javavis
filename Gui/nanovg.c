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

// Extensions to the API.

void nvgFlush(NVGcontext *ctx) {
	ctx->params.renderFlush(ctx->params.userPtr);
}

void nvgSetTransform(NVGcontext *ctx, float *tfm) {
	NVGstate *state = nvg__getState(ctx);
	memcpy(state->xform, tfm, sizeof(float)*6);
}

void nvgResizeImage(NVGcontext *ctx, int id, int w, int h) {
	GLNVGcontext *gl = (GLNVGcontext*)nvgInternalParams(ctx)->userPtr;
	GLNVGtexture *tex = glnvg__findTexture(gl, id);
	if (tex) {
		tex->width = w;
		tex->height = h;
	}
}

NVGpaint nvgImagePatternRaw(NVGcontext *ctx, float *tfm, float w, float h, int image, float alpha) {
	NVGpaint p;
	NVG_NOTUSED(ctx);
	memset(&p, 0, sizeof(p));
	memcpy(p.xform, tfm, sizeof(float)*6);
	p.extent[0] = w;
	p.extent[1] = h;
	p.image = image;
	p.innerColor = p.outerColor = nvgRGBAf(1, 1, 1, alpha);
	return p;
}


#endif
