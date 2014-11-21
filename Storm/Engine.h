#pragma once

#include "Utils/Path.h"
#include "Name.h"
#include "Package.h"
#include "Scope.h"
#include "Code/Arena.h"

namespace storm {

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

		// Get the default scope lookup for the root packag.
		inline Scope *scope() { return &rootScope; }

		// Initialized?
		inline bool initialized() { return inited; }

		// Arena.
		code::Arena arena;

	private:
		// Path to root directory.
		Path rootPath;

		// Root package.
		Package *rootPkg;

		// Create the package (recursive).
		Package *createPackage(Package *pkg, const Name &path, nat at = 0);

		// Root scope.
		Scope rootScope;

		// Cached types.
		vector<Auto<Type> > cached;

		// Initialized?
		bool inited;

		// Set T to the type, reporting any errors.
		void setType(Type *&t, const String &name);
	};

}
