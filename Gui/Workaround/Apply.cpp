#include "stdafx.h"
#include "Apply.h"
#include "StackDevice.h"
#include "Exception.h"
#include "Core/Convert.h"

namespace gui {

	static Device *apply(const char *begin, const char *end, Device *device) {
		if (begin == end)
			return device;

		std::string name(begin, end);

		if (name == "single-stack")
			return new StackDevice(device);

		Engine &e = runtime::someEngine();
		StrBuf *msg = new (e) StrBuf();
		*msg << S("Unknown render workaround found in ") S(ENV_RENDER_WORKAROUND) S(": ") << toWChar(e, name.c_str())->v;
		throw new (e) GuiError(msg->toS());
	}

	Device *applyEnvWorkarounds(Device *device) {
		const char *env = getenv(ENV_RENDER_WORKAROUND);
		if (!env)
			return device;

		const char *start = env;
		const char *at = env;
		while (true) {
			if (*at == ',') {
				device = apply(start, at, device);
				start = at + 1;
			} else if (*at == '\0') {
				device = apply(start, at, device);
				break;
			}

			at++;
		}

		return device;
	}

#ifdef GUI_GTK

	Surface *applyGLWorkarounds(Surface *surface) {
		// Don't do anything if we already have something in the workarounds variable.
		if (getenv(ENV_RENDER_WORKAROUND))
			return surface;

		const char *vendor = (const char *)glGetString(GL_VENDOR);
		const char *version = (const char *)glGetString(GL_VERSION);
		if (strcmp(vendor, "Intel") == 0) {
			// When the vendor is "Intel" (not "Intel Open Source Technology Center"), we are using
			// the IRIS driver. At least when the version string contains MESA. For versions below
			// 20.x.x, the bug is present.
		}

		return surface;
	}

#endif

}
