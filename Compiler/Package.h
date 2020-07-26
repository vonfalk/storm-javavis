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

		// Check for exact matches.
		virtual Bool STORM_FN has(Named *item);

		// Add elements and templates.
		virtual void STORM_FN add(Named *item);
		virtual void STORM_FN add(Template *item);

		// Remove items and templates.
		virtual Bool STORM_FN remove(Named *item);
		virtual Bool STORM_FN remove(Template *item);

		// Find an element.
		virtual MAYBE(Named *) STORM_FN find(SimplePart *part, Scope source);
		using NameSet::find;

		// Get iterators to the begin and end of the contents.
		virtual NameSet::Iter STORM_FN begin() const;
		virtual NameSet::Iter STORM_FN end() const;

		// Lazy-loading.
		virtual Bool STORM_FN loadName(SimplePart *part);
		virtual Bool STORM_FN loadAll();

		// Inhibit discard messages from propagating.
		void STORM_FN noDiscard();

		// We don't need to propagate the discard source message in general.
		virtual void STORM_FN discardSource();

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Reload all files in the current package.
		void STORM_FN reload();

		// Reload source code from a subset of source files. 'files' is a list of the files that
		// shall be examined, and all of them are assumed to be located in the current package. Any
		// files not in 'files' are assumed to be unchanged, and their contents will remain
		// untouched. Does not handle removals of source code, use 'reload(Array<Url>, Bool)' for that.
		void STORM_FN reload(Array<Url *> *files);

		// Reload source code from a subset of source files. 'files' is a list of the files that
		// shall be examined, and all of them are assumed to be located in the current package. Any
		// files not in 'files' are assumed to be unchanged, and their contents will remain
		// untouched. If 'complete' is true, then 'files' are assumed to contain all files (not
		// directories) in the current package. Thus, any files not in 'files' are assumed to have
		// been removed.
		void STORM_FN reload(Array<Url *> *files, Bool complete);

	private:
		// Our path. Points tu null if we're a virtual package.
		MAYBE(Url *) pkgPath;

		// NameSet we're using as a temporary storage for currently loading entities during a load
		// or a reload operation. Whenever non-null, new entities are added to this name set instead
		// of the package itself, and name queries are resolved against 'loading' if able and then
		// against the package. If 'loadingAllowDuplicates' is false, we check for duplicates before
		// adding to 'loading'.
		MAYBE(NameSet *) loading;

		// Allow duplicates when adding entities to 'loading'?
		Bool loadingAllowDuplicates;

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
