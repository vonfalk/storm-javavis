#pragma once
#include "Name.h"
#include "Utils/Path.h"
#include "Lib/Object.h"

namespace storm {
	STORM_PKG(lang);
	class PkgFiles;
	class Engine;
	class SyntaxRules;
	class Scope;

	// Get the name of the 'reader' class from 'pkg'.
	Name readerName(const Name &name);

	// Get the name of the package containing the syntax to be used when parsing 'path'.
	Name syntaxPkg(const Path &path);

	// Same as above, but generates a map of all files with the same extension.
	hash_map<Name, PkgFiles *> syntaxPkg(const vector<Path> &paths, Engine &e);

	/**
	 * Input set to a PkgReader.
	 */
	class PkgFiles : public Object {
		STORM_CLASS; // final
	public:
		STORM_CTOR PkgFiles();

		// Add a new file.
		void add(const Path &file);

		// ToS
		virtual Str *STORM_FN toS();

		// All files to process.
		vector<Path> files;
	};

	/**
	 * Load a specific file type from a package. This is the abstract base
	 * that does not do very much at all. Load syntaxPkg.Reader overload of
	 * this class to parse a language!
	 *
	 * The process is designed to be implemented as follows:
	 * 1: get syntax
	 * 2: get types (just information, should be lazy-loaded!)
	 * 3: resolve types (ie figure out how structs should look)
	 * 4: get functions
	 * This is to ensure that inter-depencies are resolved correctly. Within each language,
	 * these rules may be implemented differently.
	 */
	class PkgReader : public Object {
		STORM_CLASS;
	public:
		// Create a pkgReader.
		STORM_CTOR PkgReader(PkgFiles *files);

		// Dtor.
		~PkgReader();

		// Files passed to this object.
		PkgFiles *files;

		// Get syntax. Override to implement syntax-loading. TODO: STORM_FN, expose custom scope through package ptr.
		virtual void readSyntax(SyntaxRules &to, Scope &scope);
	};

}
