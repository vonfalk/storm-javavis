#include "stdafx.h"
#include "LoadedLibs.h"
#include "Shared/BuiltIn.h"
#include "Type.h"
#include "BuiltInLoader.h"
#include "Shared/Io/Url.h"
#include "Engine.h"
#include "Exception.h"
#include "Code/VTable.h"

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
		unload();
	}

	void LoadedLibs::shutdown() {
		for (LibMap::iterator i = loaded.begin(), end = loaded.end(); i != end; ++i)
			i->second->shutdown();
	}

	void LoadedLibs::clearTypes() {
		for (LibMap::iterator i = loaded.begin(), end = loaded.end(); i != end; ++i)
			i->second->clearTypes();
	}

	void LoadedLibs::unload() {
		for (LibMap::iterator i = loaded.begin(), end = loaded.end(); i != end; ++i)
			delete i->second;
		loaded.clear();
	}

	LibData *LoadedLibs::load(Par<Url> url) {
		LibHandle lib = storm::load(url);
		if (!lib)
			return null;
		LibMain e = libMain(lib);
		if (!e) {
			storm::unload(lib);
			return null;
		}

		LibMap::iterator i = loaded.find(lib);
		if (i == loaded.end()) {
			LibData *d = new LibData(url->engine(), lib, e);
			loaded.insert(make_pair(lib, d));
			return d;
		} else {
			i->second->newRef();
			return i->second;
		}
	}


	LibData::LibData(Engine &engine, LibHandle l, LibMain e) : engine(engine), lib(l), timesLoaded(1) {
		isShutdown = false;

		dllInterface = storm::dllInterface();
		dllInterface.builtIn = &LibData::libBuiltIn;
		dllInterface.cppVTable = &LibData::libVTable;
		dllInterface.getData = &LibData::libData;
		dllInterface.data = this;

		info = (*e)(&dllInterface);

		toSFn = code::deVirtualize(address(&Object::toS), (void *)info.builtIn->objectVTable);
		engine.addRootToS(toSFn);
	}

	LibData::~LibData() {
		// We clear types first, it is a _bad_ idea to try to execute destructors possibly in
		// the unloaded dll after we have unloaded it!
		clearTypes();

		// Free the data.
		(*info.destroyData)(info.data);

		engine.removeRootToS(toSFn);
		for (nat i = 0; i < timesLoaded; i++)
			unload(lib);
	}

	void LibData::shutdown() {
		if (!isShutdown)
			(*info.shutdownData)(info.data);
		isShutdown = true;
	}

	void LibData::clearTypes() {
		shutdown();
		types.clear();
	}

	void LibData::newRef() {
		assert(false, L"This will not be good, 'info' contains instantiation-specific data!");
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

	void *LibData::libData(Engine &e, void *data) {
		LibData *me = (LibData *)data;
		return me->info.data;
	}

	BuiltInLoader *LibData::loader(Engine &e, Package *root) {
		if (info.builtIn) {
			BuiltInLoader *loader = new BuiltInLoader(e, types, *info.builtIn, root);
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
