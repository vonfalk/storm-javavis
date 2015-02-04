#pragma once
#include "Name.h"
#include "SyntaxRule.h"
#include "Named.h"
#include "Scope.h"
#include "NameSet.h"
#include "Utils/Path.h"

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
		Package(const String &name, Engine &engine);

		// 'dir' is the directory this package is located in.
		Package(const Path &pkgPath, Engine &engine);

		// Dtor.
		~Package();

		// Owning engine.
		Engine &engine;

		// Get a list of all syntax options in this package.
		// The options are still owned by this class.
		const SyntaxRules &syntax();

		// Get parent.
		virtual NameLookup *parent() const;

	protected:
		// Find a name here.
		virtual Named *findHere(const String &name, const vector<Value> &params);

		// Output.
		virtual void output(std::wostream &to) const;

	private:
		// Our path. Points to null if this is a virtual package.
		Path *pkgPath;

		// Rules present in this package.
		SyntaxRules syntaxRules;

		/**
		 * Lazy-loading status:
		 */

		// All code files (.sto among others) examined?
		bool loaded, loading;

		// Load code if not done yet. (need something more complex when supporting re-loads).
		void load();

		// Load code unconditionally. Use load()
		void loadAlways();

		// Create a PkgReader from 'pkg'.
		PkgReader *createReader(Par<Name> pkg, Par<PkgFiles> files);

		// Add a PkgReader to 'to'.
		void addReader(vector<Auto<PkgReader> > &to, Par<Name> pkg, Par<PkgFiles> files);

		/**
		 * Init.
		 */
		void init();

		/**
		 * Loading of sub-packages.
		 */

		// Try to load a sub-package. Returns null on failure.
		Package *loadPackage(const String &name);
	};

}
