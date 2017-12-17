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

	int RenderResource::texture(Painter *owner) {
		this->owner = owner;
		if (texture() < 0) {
			int id = create(owner);
			texture(id);
			if (id >= 0)
				owner->addResource(this);
		}
		return texture();
	}

	void RenderResource::destroy() {
		if (texture() >= 0) {
			if (owner) {
				nvgDeleteImage(owner->nvgContext(), texture());
			}
			// NOTE: Destroying the nvgContext destroys the textures created there as well, so we
			// don't need to worry in case we do not have an owner.
		}
		texture(-1);
	}

	int RenderResource::create(Painter *owner) {
		return -1;
	}

#endif

}
