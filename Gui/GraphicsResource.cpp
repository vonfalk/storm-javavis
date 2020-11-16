#include "stdafx.h"
#include "GraphicsResource.h"

namespace gui {

	void GraphicsMgrRaw::create(SolidBrush *brush, void *&result, Resource::Cleanup &cleanup) {
		assert(false);
	}

	void GraphicsMgrRaw::create(LinearGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		assert(false);
	}

	void GraphicsMgrRaw::create(RadialGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		assert(false);
	}

	void GraphicsMgrRaw::update(SolidBrush *brush, void *resource) {
		assert(false);
	}

	void GraphicsMgrRaw::update(LinearGradient *brush, void *resource) {
		assert(false);
	}

	void GraphicsMgrRaw::update(RadialGradient *brush, void *resource) {
		assert(false);
	}


	void GraphicsMgr::create(SolidBrush *brush, void *&result, Resource::Cleanup &cleanup) {
		cleanup = null;
		result = create(brush);
	}

	void GraphicsMgr::create(LinearGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		cleanup = null;
		result = create(brush);
	}

	void GraphicsMgr::create(RadialGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		cleanup = null;
		result = create(brush);
	}

	void GraphicsMgr::update(SolidBrush *, void *resource) {
		((GraphicsResource *)resource)->update();
	}

	void GraphicsMgr::update(LinearGradient *, void *resource) {
		((GraphicsResource *)resource)->update();
	}

	void GraphicsMgr::update(RadialGradient *, void *resource) {
		((GraphicsResource *)resource)->update();
	}

}
