#include "stdafx.h"
#include "Graphics.h"
#include "Bitmap.h"
#include "Core/Exception.h"

namespace gui {

	Graphics::Graphics() {
		resources = new (this) WeakSet<Resource>();
	}

	Graphics::~Graphics() {
		// Things might be null in the destructor...
		if (resources) {
			WeakSet<Resource>::Iter i = resources->iter();
			while (Resource *r = i.next()) {
				if (runtime::liveObject(r))
					r->destroy(this);
			}
		}
	}

	Bool Graphics::attach(Resource *resource) {
		return resources->put(resource);
	}

	void Graphics::destroy() {
		WeakSet<Resource>::Iter i = resources->iter();
		while (Resource *r = i.next()) {
			r->destroy(this);
		}
		resources->clear();
	}

	void Graphics::reset() {
		while (pop())
			;
	}

	void Graphics::draw(Bitmap *bitmap) {
		draw(bitmap, Point());
	}

	void Graphics::draw(Bitmap *bitmap, Point topLeft) {
		draw(bitmap, Rect(topLeft, topLeft + bitmap->size()));
	}

	void Graphics::draw(Bitmap *bitmap, Point topLeft, Float opacity) {
		draw(bitmap, Rect(topLeft, topLeft + bitmap->size()), opacity);
	}

	void Graphics::draw(Bitmap *bitmap, Rect rect) {
		draw(bitmap, rect, 1);
	}

	void Graphics::draw(Bitmap *bitmap, Rect src, Point topLeft) {
		draw(bitmap, src, Rect(topLeft, topLeft + src.size()));
	}

	void Graphics::draw(Bitmap *bitmap, Rect src, Point topLeft, Float opacity) {
		draw(bitmap, src, Rect(topLeft, topLeft + src.size()), opacity);
	}

	void Graphics::draw(Bitmap *bitmap, Rect src, Rect dest) {
		draw(bitmap, src, dest, 1);
	}

	GraphicsResource *Graphics::create(SolidBrush *) {
		throw new (this) NotSupported(S("create(SolidBrush)"));
	}

	GraphicsResource *Graphics::create(LinearGradient *) {
		throw new (this) NotSupported(S("create(LinearGradient)"));
	}

	GraphicsResource *Graphics::create(RadialGradient *) {
		throw new (this) NotSupported(S("create(RadialGradient)"));
	}

}
