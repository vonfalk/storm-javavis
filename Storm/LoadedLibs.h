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

	typedef const BuiltIn *(*LibMain)(const DllInterface *);

	/**
	 * Keeps track of dynamically loaded libraries, and makes sure we unload them when we do not
	 * need them anymore (currently only at shutdown).
	 */
	class LoadedLibs : NoCopy {
	public:
		LoadedLibs();
		~LoadedLibs();

		// Clear libraries.
		void clear();

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
		LibData(LibHandle lib, LibMain fn);

		~LibData();

		// The library has been loaded again for some reason.
		void newRef();

		// Load the library. Increases the # of times loaded.
		const BuiltIn *load();

	private:
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

		// Get types for this lib. Called from the lib itself.
		static Type *libBuiltIn(Engine &e, void *data, nat id);
	};

}
