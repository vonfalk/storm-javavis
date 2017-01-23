#include "stdafx.h"
#include "RenderResource.h"
#include "Painter.h"

namespace stormgui {

	RenderResource::RenderResource() : resource(null), owner(null) {}

	RenderResource::~RenderResource() {
		::release(resource);
		if (owner)
			owner->removeResource(this);
	}

	void RenderResource::destroy() {
		::release(resource);
	}

	void RenderResource::forgetOwner() {
		owner = null;
	}

	void RenderResource::create(Painter *from, ID2D1Resource **out) {}

}