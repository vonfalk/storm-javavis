#include "stdafx.h"
#include "LoadedLibs.h"
#include "Shared/BuiltIn.h"
#include "Type.h"
#include "BuiltInLoader.h"
#include "Io/Url.h"
#include "Engine.h"
#include "Exception.h"

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

	void LoadedLibs::clearTypes() {
		for (LibMap::iterator i = loaded.begin(), end = loaded.end(); i != end; ++i)
			i->second->clearTypes();
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
		// We clear types first, it is a _bad_ idea to try to execute destructors possibly in
		// the unloaded dll after we have unloaded it!
		clearTypes();
		for (nat i = 0; i < timesLoaded; i++)
			unload(lib);
	}

	void LibData::clearTypes() {
		types.clear();
	}

	void LibData::newRef() {
		timesLoaded++;
	}

	Type *LibData::libBuiltIn(Engine &e, void *data, nat id) {
		LibData *me = (LibData *)data;
		return me->types[id].borrow();
	}

	void *LibData::libVTable(void *data, nat id) {
		LibData *me = (LibData *)data;
		return me->types[id]->vtable.baseVTable();
	}

	BuiltInLoader *LibData::loader(Engine &e, Package *root) {
		if (builtIn) {
			BuiltInLoader *loader = new BuiltInLoader(e, types, *builtIn, root);
			if (loader->vtableCapacity() > e.maxCppVTable()) {
				TODO(L"Do something better here!");
				throw BuiltInError(L"VTables in the library are too large " + ::toS(loader->vtableCapacity()));
			}
			return loader;
		} else {
			return null;
		}
	}

}
