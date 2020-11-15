#include "stdafx.h"
#include "Resource.h"
#include "Graphics.h"

namespace gui {

	Resource::Resource() : data(null), info(0) {}

	void Resource::destroy(Graphics *g) {
		GraphicsResource *r = get(g->id());
		if (r) {
			if (r->release()) {
				r->destroy();
				clear(g->id());
			}
		}
	}

	GraphicsResource *Resource::forGraphics(Graphics *g) {
		GraphicsResource *r = get(g->id());
		if (!r) {
			r = create(g);
			set(g->id(), r);
		}

		// Keep track of instances where a GraphicsResource is shared.
		if (g->attach(this))
			r->addRef();

		return r;
	}

	void Resource::update() {
		if (info == 0) {
			// Nothing to do.
		} else if ((info & 0x1) == 0) {
			// Single element.
			((GraphicsResource *)data)->update();
		} else {
			// Array.
			Arr *array = (Arr *)data;
			for (Nat i = 0; i < array->count; i++) {
				if (array->v[i])
					array->v[i]->update();
			}
		}
	}

	GraphicsResource *Resource::get(Nat id) {
		Nat offset = info >> 1;
		if (info & 0x1) {
			Arr *array = (Arr *)data;
			if (id >= offset && id < array->count + offset)
				return array->v[id - offset];
			return null;
		} else {
			if (id == offset) {
				return (GraphicsResource *)data;
			} else {
				return null;
			}
		}
	}

	void Resource::set(Nat id, GraphicsResource *r) {
		Nat offset = info >> 1;
		if (info == 0) {
			// First element.
			data = r;
			info = id << 1;
		} else if ((info & 0x1) == 0x0) {
			// There is a single element there.
			if (id == offset) {
				// Simply replace the one already there.
				data = r;
			} else {
				// We need to create an array.
				Nat nOffset = min(offset, id);
				Nat count = max(offset, id) - nOffset + 1;
				Arr *array = runtime::allocArray<GraphicsResource *>(engine(), &pointerArrayType, count);
				array->v[offset - nOffset] = (GraphicsResource *)data;
				array->v[id - nOffset] = r;
				data = array;
				info = nOffset << 1 | 0x1;
			}
		} else {
			// We already have an array, but do we need to resize it?
			Arr *array = (Arr *)data;
			if (id >= offset && id < array->count + offset) {
				// No, we can reuse it!
				array->v[id - offset] = r;
			} else {
				// Need to resize it.
				Nat nOffset = min(offset, id);
				Nat count = max(array->count + offset, id + 1) - nOffset;
				Arr *nArray = runtime::allocArray<GraphicsResource *>(engine(), &pointerArrayType, count);
				for (Nat i = 0; i < array->count; i++)
					nArray->v[i + offset - nOffset] = array->v[i];

				nArray->v[id - nOffset] = r;
				array = nArray;
				info = nOffset << 1 | 0x1;
			}
		}
	}

	void Resource::clear(Nat id) {
		Nat offset = info >> 1;
		if ((info & 0x1) == 0) {
			// Single element.
			if (id == offset) {
				data = null;
				info = 0;
			}
		} else {
			// Array.
			Arr *array = (Arr *)data;
			if (id < offset || id >= array->count + offset) {
				// Outside the range, nothing to do.
			} else if (id > offset && id < array->count + offset - 1) {
				// Not one of the edge element, no need to resize.
				array->v[id - offset] = null;
			} else {
				// One of the edge elements, we need to resize.
				array->v[id - offset] = null;

				Nat low = array->count + offset;
				Nat high = 0;
				for (Nat i = 0; i < array->count; i++) {
					if (array->v[i]) {
						low = min(low, i + offset);
						high = max(high, i + offset);
					}
				}

				if (low == high) {
					info = offset << 1;
					data = array->v[low - offset];
					return;
				}

				Arr *nArray = runtime::allocArray<GraphicsResource *>(engine(), &pointerArrayType, high - low + 1);
				for (Nat i = low; i <= high; i++) {
					nArray->v[i - low] = array->v[i - offset];
				}

				info = low << 1 | 0x1;
				array = nArray;
			}
		}
	}



	GraphicsResource::GraphicsResource() : refs(0) {}

	GraphicsResource::~GraphicsResource() {}

	void GraphicsResource::update() {}

	void GraphicsResource::destroy() {}

}
