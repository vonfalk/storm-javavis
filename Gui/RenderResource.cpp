#include "stdafx.h"
#include "RenderResource.h"
#include "Painter.h"

namespace gui {

	RenderResource::RenderResource() : resource(null), owner(null) {}

	RenderResource::~RenderResource() {
		destroy();
	}

	void RenderResource::forgetOwner() {
		owner = null;
	}

#ifdef GUI_WIN32

	void RenderResource::destroy() {
		::release(resource);
	}

	void RenderResource::create(Painter *from, ID2D1Resource **out) {}

#endif
#ifdef GUI_GTK

	void RenderResource::destroy() {
		if (resource)
			cairo_pattern_destroy(resource);
		resource = null;
	}

	cairo_pattern_t *RenderResource::create() {
		return null;
	}

#endif

}
