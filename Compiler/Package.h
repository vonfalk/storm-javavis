#pragma once
#include "NameSet.h"
#include "Core/Io/Url.h"

namespace storm {
	STORM_PKG(core.lang);

	class PkgReader;
	class PkgFiles;

	/**
	 * Defines the contents of a package. A package is a NameSet associated with a path, which
	 * allows it to load things from there.
	 */
	class Package : public NameSet {
		STORM_CLASS;
	public:
		// Create a virtual package, ie. a package that is not present on disk. These packages must
		// be eagerly loaded by 'add'ing things to them.
		STORM_CTOR Package(Str *name);

		// Create a package located in 'path'.
		STORM_CTOR Package(Url *path);

		// Create a package located in 'path' with the given name.
		STORM_CTOR Package(Str *name, Url *path);

		// Get parent.
		virtual MAYBE(NameLookup *) STORM_FN parent() const;

		// Get our url.
		virtual MAYBE(Url *) STORM_FN url() const;

		// Set url. Only done by Engine during startup and is therefore not exposed to storm.
		void setUrl(Url *to);

		// Lazy-loading.
		virtual Bool STORM_FN loadName(SimplePart *part);
		virtual Bool STORM_FN loadAll();

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Our path. Points tu null if we're a virtual package.
		Url *pkgPath;

		/**
		 * Loading of sub-packages.
		 */

		// Load all files given.
		void loadFiles(Array<Url *> *files);

		// Create a reader for each element in 'readers'.
		Array<PkgReader *> *createReaders(Map<SimpleName *, PkgFiles *> *readers);

		// Try to load a sub-package. Returns null on failure.
		Package *loadPackage(Str *name);
	};

	/**
	 * Documentation for a package.
	 *
	 * Looks for a file named README and uses the text in that file as documentation.
	 */
	class PackageDoc : public NamedDoc {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR PackageDoc(Package *pkg);

		// Generate documentation.
		virtual Doc *STORM_FN get();

	private:
		// Owner.
		Package *pkg;
	};


	// Find a package from a path.
	MAYBE(Package *) STORM_FN package(Url *path);

	// Get the root package.
	Package *STORM_FN rootPkg(EnginePtr e);

}
