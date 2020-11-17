#include "stdafx.h"
#include "GraphicsMgr.h"
#include "Core/Exception.h"

namespace gui {

	DEFINE_GRAPHICS_MGR_FNS(GraphicsMgrRaw);

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
