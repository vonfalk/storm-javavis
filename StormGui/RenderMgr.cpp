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
		factory = null;
		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &options, (void **)&factory);
	}

	RenderMgr::~RenderMgr() {
		::release(factory);
	}

	RenderMgr *renderMgr(EnginePtr e) {
		LibData *d = e.v.data();
		if (!d->renderMgr)
			d->renderMgr = CREATE(RenderMgr, e.v);
		return d->renderMgr.ret();
	}

}
