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
	 *
	 * TODO: Unify loading of syntax and code into one (syntax may be present in code files as well!)
	 */
	class Package : public Named, public NameLookup {
	public:
		// Create a virtual package, ie a package not present
		// on disk. Those packages must therefore be eagerly loaded.
		Package(const String &name);

		// 'dir' is the directory this package is located in.
		Package(const Path &pkgPath);

		// Dtor.
		~Package();

		// Get a list of all syntax options in this package.
		// The options are still owned by this class.
		hash_map<String, SyntaxRule*> syntax();

		// Get a sub-package by name.
		Package *childPackage(const String &name);

		// Add a sub-package. Assumes that it does not already exists!
		void add(Package *pkg, const String &name);

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
		typedef hash_map<String, Package*> PkgMap;
		PkgMap packages;

		// Rules present in this package.
		typedef hash_map<String, SyntaxRule*> SyntaxMap;
		SyntaxMap syntaxRules;

		// Types in this package.
		typedef hash_map<String, Type*> TypeMap;
		TypeMap types;

		// All functions and variables in this package.
		typedef hash_map<String, Overload*> MemberMap;
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

		// All syntax (.bnf-files) examined?
		bool syntaxLoaded;

		// All code files (.sto among others) examined?
		bool codeLoaded;

		/**
		 * Init.
		 */
		void init();

		/**
		 * Loading of sub-packages.
		 */

		// Try to load a sub-package. Returns null on failure.
		Package *loadPackage(const String &name);

		// Load all syntax found in this package.
		void loadSyntax();
	};

}
