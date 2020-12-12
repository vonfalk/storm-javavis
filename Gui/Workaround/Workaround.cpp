#include "stdafx.h"
#include "Workaround.h"

namespace gui {

	SurfaceWorkaround::SurfaceWorkaround(SurfaceWorkaround *prev) : prev(prev) {}

	SurfaceWorkaround::~SurfaceWorkaround() {
		delete prev;
	}

	Surface *SurfaceWorkaround::apply(Surface *to) {
		if (prev)
			to = prev->apply(to);
		return applyThis(to);
	}


	WorkaroundDevice::WorkaroundDevice(Device *wrap, SurfaceWorkaround *workarounds)
		: wrap(wrap), workarounds(workarounds) {}

	WorkaroundDevice::~WorkaroundDevice() {
		delete wrap;
		delete workarounds;
	}

	Surface *WorkaroundDevice::createSurface(Handle window) {
		Surface *r = wrap->createSurface(window);
		if (!r)
			return r;

		return workarounds->apply(r);
	}

	TextMgr *WorkaroundDevice::createTextMgr() {
		return wrap->createTextMgr();
	}

}
