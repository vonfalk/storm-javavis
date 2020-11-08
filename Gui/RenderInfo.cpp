#include "stdafx.h"
#include "RenderInfo.h"
#include "D2DGraphics.h"
#include "CairoGraphics.h"
#include "CairoDevice.h"

namespace gui {

#ifdef GUI_WIN32

	WindowGraphics *RenderInfo::createGraphics(Engine &e) const {
		return new (e) D2DGraphics(*this);
	}

	void RenderInfo::release() {
		if (target()) {
			target()->Release();
			target(null);
		}

		if (swapChain()) {
			swapChain()->Release();
			swapChain(null);
		}
	}

#endif
#ifdef GUI_GTK

	WindowGraphics *RenderInfo::createGraphics(Engine &e) const {
		// TODO: We need to ask the device here.
		return new (e) CairoGraphics(*this);
	}

	void RenderInfo::release() {
		if (target()) {
			// 'target' is owned by the surface nowadays.
			// cairo_destroy(target());
			target(null);
		}

		if (surface()) {
			delete surface();
			surface(null);
		}
	}

#endif
}
