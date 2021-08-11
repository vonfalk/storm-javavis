#pragma once
#include "NameSet.h"
#include "Core/Io/Url.h"

namespace storm {
	STORM_PKG(core.lang);

	class PkgReader;
	class PkgFiles;

	class PackageExports;

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

		// Add an export. That is: add a package that will be automatically included when this
		// package is included. The key used here can be used to indicate that a particular package
		// shall only be included in particular circumstances, e.g. for a particular language.
		void STORM_FN addExport(Package *pkg);
		void STORM_FN addExport(Package *pkg, MAYBE(Package *) key);

		// Get exports. Either add them to an existing array, or get an array.
		Array<Package *> *STORM_FN exports(MAYBE(Package *) key);
		void STORM_FN exports(MAYBE(Package *) key, Array<Package *> *to);
		void STORM_FN exports(MAYBE(Package *) key, Array<NameLookup *> *to);

		// Inhibit discard messages from propagating.
		void STORM_FN noDiscard();

		// We don't need to propagate the discard source message in general.
		virtual void STORM_FN discardSource();

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Our path. Points to null if we're a virtual package.
		Url *pkgPath;

		// General exports (i.e. for all types of users).
		PackageExports *generalExports;

		// Exports specific to some language. We maintain the invariant that exports in the general
		// export do not appear here.
		Map<Package *, PackageExports *> *specificExports;

		// Shall we emit 'discardSource' messages on load?
		Bool discardOnLoad;

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
	 * Package exports.
	 *
	 * Contains a list of packages that are exported by a package (i.e. packages that are
	 * automatically included when a package is exported.
	 */
	class PackageExports : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR PackageExports();

		// Copy.
		PackageExports(const PackageExports &src);

		// Add an export.
		Bool STORM_FN push(Package *package);

		// Remove an export.
		Bool STORM_FN remove(Package *package);

		// Append the content in here to another array.
		void STORM_FN append(Array<Package *> *to);

	private:
		// Exports.
		Array<Package *> *exports;
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
