#pragma once
#include "PkgPath.h"
#include "SyntaxType.h"
#include "Utils/Path.h"

namespace storm {

	/**
	 * Defines the contents of a package. A package may contain
	 * one or more of the following items:
	 * - Classes (ie types)
	 * - Functions
	 * - Syntax rules
	 * - Packages
	 * The Package instance is expected to live as long as the
	 * engine object, and is therefore managed by the engine object.
	 * Packages are lazily loaded, which means that the contents
	 * is not loaded until it is needed.
	 */
	class Package : public Printable, NoCopy {
	public:
		// Create a virtual package, ie a package not present
		// on disk. Those packages must therefore be eagerly loaded.
		Package();

		// 'dir' is the directory this package is located in.
		Package(const Path &pkgPath);

		~Package();

		// Find a sub-package. Returns 'null' on failure.
		Package *find(const PkgPath &path, nat start = 0);

		// Get a list of all syntax rules in this package.
		// The rules are still owned by this class.
		hash_map<String, SyntaxType*> syntax();

	protected:
		virtual void output(std::wostream &to) const;
	private:
		// Our path. Points to null if this is a virtual package.
		Path *pkgPath;

		// Sub-packages. These are loaded on demand.
		typedef hash_map<String, Package*> PkgMap;
		PkgMap packages;

		// Rules present in this package.
		typedef hash_map<String, SyntaxType*> SyntaxMap;
		SyntaxMap syntaxTypes;

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
