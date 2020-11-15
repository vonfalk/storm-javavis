#include "stdafx.h"
#include "Device.h"
#include "D2D/D2D.h"

namespace gui {

#if defined(GUI_WIN32)

	Device *Device::create(Engine &e) {
		return new D2DDevice(e);
	}

#elif defined(GUI_GTK)

	Device *Device::create(Engine &e) {
		assert(false, L"TODO: Fixme!");
	}

#else
#error "Unknown UI toolkit."
#endif

}
