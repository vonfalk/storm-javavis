#include "stdafx.h"
#include "DynamicLibs.h"
#include "Shared/BuiltIn.h"
#include "Io/Url.h"

namespace storm {

#ifdef WINDOWS

	static const DynamicLib invalid = NULL;

	DynamicLib load(Par<Url> url) {
		return LoadLibrary(url->format().c_str());
	}

	const BuiltIn *initialize(DynamicLib lib, const DllInterface *interface) {
		String name = STRING(ENTRY_POINT_NAME);

		typedef const BuiltIn *(*Fn)(const DllInterface *);
		Fn fn = (Fn)GetProcAddress(lib, name.toChar().c_str());
		if (!fn)
			return null;

		return (*fn)(interface);
	}

	void unload(DynamicLib lib) {
		FreeLibrary(lib);
	}

#endif

	DynamicLibs::DynamicLibs() {}

	DynamicLibs::~DynamicLibs() {
		for (nat i = 0; i < loaded.size(); i++)
			unload(loaded[i]);
	}

	const BuiltIn *DynamicLibs::load(Par<Url> url) {
		DynamicLib lib = storm::load(url);
		if (!lib)
			return null;
		loaded.push_back(lib);

		DllInterface interface = dllInterface();
		return initialize(lib, &interface);
	}

}
