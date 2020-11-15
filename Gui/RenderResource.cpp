#include "stdafx.h"
#include "RenderResource.h"
#include "Painter.h"

namespace gui {

	RenderResource::RenderResource() : resource(null), destroyFn(null), owner(null) {}

	RenderResource::~RenderResource() {
		destroy();
	}

	void RenderResource::forgetOwner() {
		owner = null;
	}

	void RenderResource::attachTo(Painter *to) {
		// to->addResource(this);
	}

#ifdef GUI_WIN32

	void RenderResource::destroy() {
		::release(resource);
	}

	void RenderResource::create(Painter *from, ID2D1Resource **out) {}

#endif
#ifdef GUI_GTK

	void RenderResource::destroy() {
		if (resource && destroyFn)
			(*destroyFn)(resource);
		resource = null;
	}

	OsResource *RenderResource::create(Painter *owner) {
		return null;
	}

#endif

}
