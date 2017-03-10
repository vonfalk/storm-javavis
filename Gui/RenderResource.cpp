#include "stdafx.h"
#include "RenderResource.h"
#include "Painter.h"

namespace gui {

	RenderResource::RenderResource() : resource(null), owner(null) {}

	RenderResource::~RenderResource() {
		::release(resource);
	}

	void RenderResource::destroy() {
		::release(resource);
	}

	void RenderResource::forgetOwner() {
		owner = null;
	}

	void RenderResource::create(Painter *from, ID2D1Resource **out) {}

}
