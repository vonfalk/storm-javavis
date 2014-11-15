#pragma once
#include "Name.h"
#include "SyntaxRule.h"
#include "Named.h"
#include "Scope.h"
#include "Overload.h"
#include "Utils/Path.h"

namespace storm {

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
	class Package : public Named {
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

		// Get a sub-package by name.
		Package *childPackage(const String &name);

		// Add a sub-package. Assumes that it does not already exists!
		void STORM_FN add(Auto<Package> pkg);

		// Add a type to this package.
		void add(Type *type);

		// Add a function to this package.
		void add(NameOverload *function);

		// Find a name here.
		virtual Named *find(const Name &name);

		// Get our path relative the root.
		Name path() const;

		// Get parent.
		virtual Package *parent() const { return parentPkg; }

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// Our parent, null if root.
		Package *parentPkg;

		// Our path. Points to null if this is a virtual package.
		Path *pkgPath;

		// Sub-packages. These are loaded on demand.
		typedef hash_map<String, Auto<Package> > PkgMap;
		PkgMap packages;

		// Rules present in this package.
		SyntaxRules syntaxRules;

		// Types in this package.
		typedef hash_map<String, Auto<Type> > TypeMap;
		TypeMap types;

		// All functions and variables in this package.
		typedef hash_map<String, Auto<Overload> > MemberMap;
		MemberMap members;

		/**
		 * Recursive member lookup.
		 */

		// Find a name in this package or a sub-package.
		Named *findName(const Name &name, nat start);

		// Find the name in this package. Examines the types and functions present here.
		Named *findTypeOrFn(const Name &name, nat start);

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
		PkgReader *createReader(const Name &pkg, PkgFiles *files);

		// Add a PkgReader to 'to'.
		void addReader(vector<PkgReader *> &to, const Name &pkg, PkgFiles *files);

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
