#include "stdafx.h"
#include "LoadedLibs.h"
#include "Shared/BuiltIn.h"
#include "Type.h"
#include "Io/Url.h"

namespace storm {

#ifdef WINDOWS

	static const LibHandle invalid = NULL;

	LibHandle load(Par<Url> url) {
		return LoadLibrary(url->format().c_str());
	}

	LibMain libMain(LibHandle lib) {
		String name = STRING(ENTRY_POINT_NAME);

		typedef const BuiltIn *(*Fn)(const DllInterface *);
		return (LibMain)GetProcAddress(lib, name.toChar().c_str());
	}

	void unload(LibHandle lib) {
		FreeLibrary(lib);
	}

#endif

	LoadedLibs::LoadedLibs() {}

	LoadedLibs::~LoadedLibs() {
		clear();
	}

	void LoadedLibs::clear() {
		for (LibMap::iterator i = loaded.begin(), end = loaded.end(); i != end; ++i)
			delete i->second;
		loaded.clear();
	}

	LibData *LoadedLibs::load(Par<Url> url) {
		LibHandle lib = storm::load(url);
		if (!lib)
			return null;
		LibMain e = libMain(lib);
		if (!e)
			return null;

		LibMap::iterator i = loaded.find(lib);
		if (i == loaded.end()) {
			LibData *d = new LibData(lib, e);
			loaded.insert(make_pair(lib, d));
			return d;
		} else {
			i->second->newRef();
			return i->second;
		}
	}


	LibData::LibData(LibHandle l, LibMain e) : lib(l), timesLoaded(1) {
		dllInterface = storm::dllInterface();
		dllInterface.builtIn = &LibData::libBuiltIn;
		dllInterface.data = this;

		builtIn = (*e)(&dllInterface);
	}

	LibData::~LibData() {
		for (nat i = 0; i < timesLoaded; i++)
			unload(lib);
	}

	void LibData::newRef() {
		timesLoaded++;
	}

	Type *LibData::libBuiltIn(Engine &e, void *data, nat id) {
		LibData *me = (LibData *)data;
		return me->types[id].borrow();
	}

}
