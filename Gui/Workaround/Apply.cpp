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
			return new WorkaroundDevice(device, new StackWorkaround(null));

		Engine &e = runtime::someEngine();
		StrBuf *msg = new (e) StrBuf();
		*msg << S("Unknown render workaround found in ") S(ENV_RENDER_WORKAROUND) S(": ") << toWChar(e, name.c_str())->v;
		throw new (e) GuiError(msg->toS());
	}

	Device *applyEnvWorkarounds(Device *device) {
#ifdef WINDOWS
		// Apparently, getenv is unsafe on Windows. The returned pointer may be invalidated whenever
		// someone calls getenv again. As we are in a multithreaded environment, that might happen
		// at any time, so we use _dupenv_s as suggested.
		std::string envval;
		{
			char *value = "";
			size_t len = 0;
			_dupenv_s(&value, &len, ENV_RENDER_WORKAROUND);

			// Note found?
			if (value == null)
				return device;

			envval = value;
		}
		const char *env = envval.c_str();
#else
		const char *env = getenv(ENV_RENDER_WORKAROUND);
		if (!env)
			return device;
#endif

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

	// Do we need the stack fix for the IRIS driver? Checks if the version is earlier than 20.2.5 or 20.3.1
	static bool needIrisStackFix(int major, int minor, int revision) {
		if (major > 20)
			return false;
		if (major < 20)
			return true;

		if (minor > 3)
			return false;
		if (minor < 2)
			return true;

		if (minor == 2 && revision < 5)
			return true;
		if (minor == 3 && revision < 1)
			return true;

		return false;
	}

	static SurfaceWorkaround *applyIrisWorkarounds(SurfaceWorkaround *result, const char *version) {
		const char *mesa = strstr(version, "Mesa");
		if (!mesa)
			return result;

		std::istringstream ver(mesa + 5);
		int major, minor, revision;
		char skip1, skip2;
		ver >> major >> skip1 >> minor >> skip2 >> revision;

		if (!ver || skip1 != '.' || skip2 != '.')
			return result;

		if (!needIrisStackFix(major, minor, revision))
			return result;

		return new StackWorkaround(result);
	}

	SurfaceWorkaround *glWorkarounds() {
		// Don't do anything if we already have something in the workarounds variable.
		if (getenv(ENV_RENDER_WORKAROUND))
			return null;

		SurfaceWorkaround *result = null;

		const char *vendor = (const char *)glGetString(GL_VENDOR);
		const char *version = (const char *)glGetString(GL_VERSION);
		if (strcmp(vendor, "Intel") == 0) {
			// When the vendor is "Intel" (not "Intel Open Source Technology Center"), we are using
			// the IRIS driver. At least when the version string contains "Mesa". For versions below
			// 20.x.x, the bug is present.
			result = applyIrisWorkarounds(result, version);
		}

		return result;
	}

#endif

}
