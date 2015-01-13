#pragma once

#include "Utils/Path.h"
#include "Name.h"
#include "Package.h"
#include "Scope.h"
#include "Code/Arena.h"
#include "Code/Binary.h"

namespace storm {

	/**
	 * Some special types.
	 */
	enum Special {
		specialInt = 0,
		specialNat,
		specialBool,

		// last
		specialCount,
	};

	/**
	 * Defines the root object of the compiler. This object contains
	 * everything needed by the compiler itself, and shall be kept alive
	 * as long as anything from the compiler is used.
	 * Two separate instances of this class can be used as two completely
	 * separate runtime environments.
	 */
	class Engine : NoCopy {
	public:
		// Create the engine. 'root' is the location of the root package
		// directory on disk. The package 'core' is assumed to be found
		// as a subdirectory of the given root path.
		Engine(const Path &root);

		~Engine();

		// Find the given package. Returns null on failure. If 'create' is
		// true, then all packages that does not yet exist are created.
		Package *package(const Name &path, bool create = false);

		// Get a built-in type.
		inline Type *builtIn(nat id) const { return cached[id].borrow(); }

		// Special built-in types.
		inline Type *specialBuiltIn(Special t) const { return specialCached[nat(t)].borrow(); }
		void setSpecialBuiltIn(Special t, Auto<Type> z);

		// Get the default scope lookup.
		inline Auto<ScopeLookup> scopeLookup() { assert(defScopeLookup); return defScopeLookup; }

		// Get the default scope for the root package.
		inline Scope *scope() { assert(rootScope != null); return rootScope; }

		// Initialized?
		inline bool initialized() { return inited; }

		// Arena.
		code::Arena arena;

		// Reference to the addRef and release functions.
		code::RefSource &addRef, &release;

		// Reference to the memory allocation function.
		code::RefSource allocRef, freeRef;

		// Other references.
		code::RefSource lazyCodeFn;

		// Delete old code later.
		void destroy(code::Binary *binary);

	private:
		// Path to root directory.
		Path rootPath;

		// Root package.
		Auto<Package> rootPkg;

		// Create the package (recursive).
		Package *createPackage(Package *pkg, const Name &path, nat at = 0);

		// Root scope.
		Scope *rootScope;

		// Scope lookup.
		Auto<ScopeLookup> defScopeLookup;

		// Cached types.
		vector<Auto<Type> > cached;
		vector<Auto<Type> > specialCached;

		// Initialized?
		bool inited;

		// Binary objects to destroy. TODO: Be more eager!
		vector<code::Binary *> toDestroy;

		// Set T to the type, reporting any errors.
		void setType(Type *&t, const String &name);
	};

}
