#pragma once

#include "Utils/Path.h"
#include "Name.h"
#include "Package.h"

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

	private:
		// Path to root directory.
		Path rootPath;

		// Root package.
		Package rootPkg;

		// Create the package (recursive).
		Package *createPackage(Package *pkg, const Name &path, nat at = 0);

	};

}
