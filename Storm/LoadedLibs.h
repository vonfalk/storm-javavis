#pragma once
#include "Shared/Auto.h"
#include "Shared/DllInterface.h"

namespace storm {
#ifdef WINDOWS
	typedef HMODULE LibHandle;
#else
#error "Please implement dynamic libraries for your platform here."
#endif

	class Url;
	class LibData;
	struct BuiltIn;
	class BuiltInLoader;
	class Package;

	typedef const BuiltIn *(*LibMain)(const DllInterface *);

	/**
	 * Keeps track of dynamically loaded libraries, and makes sure we unload them when we do not
	 * need them anymore (currently only at shutdown).
	 *
	 * TODO: If two engines in the same process loads the same lib, that lib will be confused of
	 * what its data pointer should be. Fix this!
	 */
	class LoadedLibs : NoCopy {
	public:
		LoadedLibs();
		~LoadedLibs();

		// Clear libraries.
		void clear();

		// Clear loaded types.
		void clearTypes();

		// Load a library. (borrowed ptr).
		LibData *load(Par<Url> lib);

	private:
		// Loaded libraries.
		typedef map<LibHandle, LibData *> LibMap;
		LibMap loaded;
	};


	/**
	 * Data for a specific library.
	 */
	class LibData : NoCopy {
	public:
		LibData(Engine &engine, LibHandle lib, LibMain fn);

		~LibData();

		// Clear our type list.
		void clearTypes();

		// The library has been loaded again for some reason.
		void newRef();

		// Get a BuiltInLoader for this library. Caller owns the loader. Not intended to be done multiple times.
		BuiltInLoader *loader(Engine &e, Package *root);

	private:
		Engine &engine;

		// The library itself.
		LibHandle lib;

		// # of times the library has been loaded.
		nat timesLoaded;

		// DllInterface for us. NOTE: 'interface' seems to be reserved by MSVC in this context.
		DllInterface dllInterface;

		// Built in functions in this lib.
		const BuiltIn *builtIn;

		// Types inserted from here.
		vector<Auto<Type>> types;

		// The ToS-function from this DLL.
		void *toSFn;

		// Get types for this lib. Called from the lib itself.
		static Type *libBuiltIn(Engine &e, void *data, nat id);

		// Get vtables for this lib. Called from the lib itself.
		static void *libVTable(void *data, nat id);
	};

}
