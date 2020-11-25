#include "stdafx.h"
#include "Device.h"
#include "D2D/D2D.h"
#include "Cairo/Cairo.h"
#include "Exception.h"

namespace gui {

#if defined(GUI_WIN32)

	Device *Device::create(Engine &e) {
		return new D2DDevice(e);
	}

#elif defined(GUI_GTK)

	Device *Device::create(Engine &e) {
		const char *preference = getenv(RENDER_ENV_NAME);
		if (!preference)
			preference = "gl"; // TODO: Perhaps Skia?

		if (strcmp(preference, "sw") == 0) {
			return new CairoSwDevice(e);
		} else if (strcmp(preference, "software") == 0) {
			return new CairoSwDevice(e);
		} else if (strcmp(preference, "gtk") == 0) {
			return new CairoGtkDevice(e);
		} else if (strcmp(preference, "gl") == 0) {
			return new CairoGlDevice(e);
		} else if (strcmp(preference, "skia") == 0) {
			// return new SkiaDevice(e);
		}

		throw new (e) GuiError(S("The supplied value of ") S(RENDER_ENV_NAME) S(" is not supported."));
	}

#else
#error "Unknown UI toolkit."
#endif

}
