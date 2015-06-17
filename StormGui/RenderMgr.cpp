#include "stdafx.h"
#include "RenderMgr.h"
#include "StormGui.h"

namespace stormgui {

#ifdef DEBUG
	static D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_INFORMATION };
#else
	static D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_NONE };
#endif

	RenderMgr::RenderMgr() {
		// Start COM for this thread.
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);

		factory = null;
		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &options, (void **)&factory);
	}

	RenderMgr::~RenderMgr() {
		::release(factory);

		CoUninitialize();
	}

	ID2D1HwndRenderTarget *RenderMgr::attach(Par<Painter> painter, HWND window) {
		painters.insert(painter.borrow());

		RECT c;
		GetClientRect(window, &c);
		D2D1_SIZE_U size = {
			c.right - c.left,
			c.bottom - c.top,
		};
		D2D1_HWND_RENDER_TARGET_PROPERTIES render = {
			window,
			size,
			D2D1_PRESENT_OPTIONS_NONE, // Try _RETAIN as well.
		};

		ID2D1HwndRenderTarget *target;
		factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), render, &target);
		return target;
	}

	void RenderMgr::detach(Par<Painter> painter) {
		painters.erase(painter.borrow());
	}

	RenderMgr *renderMgr(EnginePtr e) {
		LibData *d = e.v.data();
		if (!d->renderMgr)
			d->renderMgr = CREATE(RenderMgr, e.v);
		return d->renderMgr.ret();
	}

}
