#include "stdafx.h"
#include "Bitmap.h"
#include "Painter.h"
#include "GraphicsMgr.h"
#include "Exception.h"

namespace gui {

	Bitmap::Bitmap(Image *img) : src(img) {}

	void Bitmap::create(GraphicsMgrRaw *g, void *&result, Cleanup &update) {
		g->create(this, result, update);
	}

	void Bitmap::update(GraphicsMgrRaw *g, void *resource) {
		g->update(this, resource);
	}

	Size Bitmap::size() {
		return src->size();
	}

}
