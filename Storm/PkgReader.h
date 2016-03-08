#pragma once
#include "Name.h"
#include "Lib/Object.h"
#include "Shared/TObject.h"
#include "Shared/Map.h"
#include "NamedThread.h"
#include "Shared/Io/Url.h"

namespace storm {
	STORM_PKG(lang);
	class PkgFiles;
	class Engine;
	class SyntaxRules;
	class Package;

	// Get the name of the 'reader' class from 'pkg'.
	SimpleName *STORM_FN readerName(Par<SimpleName> name);

	// Get the name of the package containing the syntax to be used when parsing 'path'.
	SimpleName *STORM_FN syntaxPkg(Par<Url> path);

	// Same as above, but generates a map of all files with the same extension.
	MAP_PP(SimpleName, PkgFiles) *syntaxPkg(Auto<ArrayP<Url>> paths);

	/**
	 * Input set to a PkgReader.
	 * TODO? Replace with simple array?
	 */
	class PkgFiles : public ObjectOn<Compiler> {
		STORM_CLASS; // final
	public:
		STORM_CTOR PkgFiles();

		// Add a new file.
		void STORM_FN add(Par<Url> file);

		// All files to process.
		Auto<ArrayP<Url>> files;

	protected:
		virtual void output(wostream &to) const;
	};

	/**
	 * Load a specific file type from a package. This is the abstract base
	 * that does not do very much at all. Load syntaxPkg.Reader overload of
	 * this class to parse a language!
	 *
	 * The process is designed to be implemented as follows:
	 * 1: get syntax rules
	 * 2: get syntax options
	 * 3: get types (just information, should be lazy-loaded!)
	 * 4: resolve types (ie set up inheritance, figure out how structs look...)
	 * 5: get functions
	 * This is to ensure that inter-depencies are resolved correctly. Within each language,
	 * these rules may be implemented differently.
	 */
	class PkgReader : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create a pkgReader.
		STORM_CTOR PkgReader(Par<PkgFiles> files, Par<Package> owner);

		// Dtor.
		~PkgReader();

		// Owning package.
		STORM_VAR Auto<Package> pkg;

		// Files passed to this object.
		STORM_VAR Auto<PkgFiles> pkgFiles;

		// Get syntax. Override to implement syntax-loading.
		virtual void STORM_FN readSyntaxRules();

		// Get syntax options. Override to implement syntax-loading.
		virtual void STORM_FN readSyntaxOptions();

		// Get all types. Override to implement type-loading.
		virtual void STORM_FN readTypes();

		// Resolve types.
		virtual void STORM_FN resolveTypes();

		// Get all functions.
		virtual void STORM_FN readFunctions();
	};


	/**
	 * Single file.
	 */
	class FileReader : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create a FileReader.
		STORM_CTOR FileReader(Par<Url> file, Par<Package> into);

		// File.
		STORM_VAR Par<Url> file;

		// Package we're loading into.
		STORM_VAR Auto<Package> pkg;

		// Read syntax from this file.
		virtual void STORM_FN readSyntaxRules();

		// Read syntax from this file.
		virtual void STORM_FN readSyntaxOptions();

		// Read types from this file.
		virtual void STORM_FN readTypes();

		// Resolve types in this file.
		virtual void STORM_FN resolveTypes();

		// Read types from this file.
		virtual void STORM_FN readFunctions();

		// Get the package where the syntax for the current file is located.
		// TODO: Move?
		Package *syntaxPackage() const;
	};


	/**
	 * Extension to handle a set of files more or less independently. A more
	 * intelligent extension to the PkgReader above.
	 */
	class FilesReader : public PkgReader {
		STORM_CLASS;
	public:
		STORM_CTOR FilesReader(Par<PkgFiles> files, Par<Package> pkg);

		// Read contents from all files.
		virtual void STORM_FN readSyntaxRules();
		virtual void STORM_FN readSyntaxOptions();
		virtual void STORM_FN readTypes();
		virtual void STORM_FN resolveTypes();
		virtual void STORM_FN readFunctions();

		// Create a file object. This is called as late as possible.
		virtual FileReader *STORM_FN createFile(Par<Url> path);

	protected:
		// Store files while in use.
		vector<Auto<FileReader> > files;

		// Populate 'files' if it is not done already.
		void loadFiles();
	};

}
