#pragma once
#include "Name.h"
#include "Named.h"
#include "Scope.h"
#include "NameSet.h"

namespace storm {
	STORM_PKG(core.lang);

	class Type;
	class Function;
	class PkgReader;
	class PkgFiles;

	/**
	 * Defines the contents of a package. A package may contain
	 * one or more of the following items:
	 * - Types (ie classes ...)
	 * - Functions
	 * - Syntax rules
	 * - Packages
	 * The Package instance is expected to live as long as the
	 * engine object, and is therefore managed by the engine object.
	 * Packages are lazily loaded, which means that the contents
	 * is not loaded until it is needed.
	 */
	class Package : public NameSet {
		STORM_CLASS;
	public:
		// Create a virtual package, ie a package not present
		// on disk. Those packages must therefore be eagerly loaded.
		Package(const String &name);

		// 'dir' is the directory this package is located in.
		Package(Par<Url> pkgPath);

		// Dtor.
		~Package();

		// Get parent.
		virtual NameLookup *parent() const;

		// Get our url.
		virtual MAYBE(Url) *STORM_FN url() const;

		// Lazy-loading.
		virtual Named *STORM_FN loadName(Par<SimplePart> part);
		virtual Bool STORM_FN loadAll();

	protected:
		// Output.
		virtual void output(std::wostream &to) const;

	private:
		// Our path. Points to null if this is a virtual package.
		Auto<Url> pkgPath;

		// Create a PkgReader from 'pkg'.
		PkgReader *createReader(Par<SimpleName> pkg, Par<PkgFiles> files);

		// Add a PkgReader to 'to'.
		void addReader(vector<Auto<PkgReader> > &to, Par<SimpleName> pkg, Par<PkgFiles> files);

		/**
		 * Loading of sub-packages.
		 */

		// Try to load a sub-package. Returns null on failure.
		Package *loadPackage(const String &name);

		// Load all files in the package.
		void loadFiles(Auto<ArrayP<Url>> children);
	};

	// Find the root package.
	Package *STORM_ENGINE_FN rootPkg(EnginePtr e) ON(Compiler);

}