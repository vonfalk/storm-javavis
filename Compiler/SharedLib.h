#pragma once
#include "Core/Io/Url.h"
#include "Core/SharedLib.h"
#include "SharedOS.h"
#include "World.h"
#include "CppLoader.h"

namespace storm {
	STORM_PKG(core.lang);

	class Thread;
	class Package;


	/**
	 * Represents a single loaded shared library.
	 */
	class SharedLib : NoCopy {
	public:
		// Load and initialize a library. Returns null on failure.
		static SharedLib *load(Url *file);

		// Destroy.
		~SharedLib();

		// Signal to this library that shutdown is in progress.
		void shutdown();

		// Any previous instance of this library?
		SharedLib *prevInstance();

		// Create a CppLoader for this shared library.
		CppLoader createLoader(Engine &e, Package *into);

	private:
		// Entry point type.
		typedef SharedLibInfo *(*EntryFn)(const SharedLibStart *);

		// Create and initialize a library.
		SharedLib(Url *file, LoadedLib lib, EntryFn entry);

		// The OS handle to the library.
		LoadedLib lib;

		// Information about the library.
		SharedLibInfo *info;

		// The world loaded by this shared library.
		World world;

		// Implementation of the unique functions.
		static Type *cppType(Engine &e, void *lib, Nat id);
		static Type *cppTemplate(Engine &e, void *lib, Nat id, Nat count, va_list params);
		static Thread *getThread(Engine &e, void *lib, const DeclThread *decl);
	};

}
