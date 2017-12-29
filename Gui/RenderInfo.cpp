#include "stdafx.h"
#include "RenderInfo.h"
#include "GlDevice.h"

namespace gui {

#ifdef GUI_WIN32

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

	void RenderInfo::release() {
		if (target()) {
			cairo_destroy(target());
			target(null);
		}

		if (surface()) {
			delete surface();
			surface(null);
		}
	}

#endif
}
