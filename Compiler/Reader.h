#pragma once
#include "Core/Array.h"
#include "Core/Fn.h"
#include "Core/Io/Url.h"
#include "NamedThread.h"
#include "Package.h"

namespace storm {
	STORM_PKG(lang);

	/**
	 * Wrapper of an array of URL:s, since we need to be able to express a map of those. This is not
	 * allowed by the preprocessor unless we wrap it inside a class.
	 */
	class PkgFiles : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR PkgFiles();

		virtual void STORM_FN deepCopy(CloneEnv *e);

		Array<Url *> *files;
	};

	// Get the package containing syntax when parsing 'file'. This is the default package, and
	// languages may choose to ignore this.
	SimpleName *STORM_FN syntaxPkgName(Url *file);

	// Get the name of the 'reader' function for use when reading 'file'.
	SimpleName *STORM_FN readerName(Url *file);

	// Group files together by which reader they should use. Storm will have a custom implementation
	// of this which does not use a 'PkgFiles' object.
	Map<SimpleName *, PkgFiles *> *readerName(Array<Url *> *files);

	/**
	 * Load a specific file type from a package. This is the abstract base class that does not do
	 * very much at all. Create a function 'lang.ext.reader(Array<Url>, Package)' which creates an
	 * instance of a Reader, and you are ready to parse a language!
	 *
	 * The process is designed to be implemented as follows:
	 * 1: get syntax rules
	 * 2: get syntax options
	 * 3: get types (just information, should be lazily-loaded!)
	 * 4: resolve types (ie. set up inheritance, figure out how structs look...)
	 * 5: get functions
	 *
	 * This is to ensure that inter-dependencies are resolved correctly. Within each language, these
	 * rules may be implemented differently.
	 */
	class PkgReader : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create a PkgReader.
		STORM_CTOR PkgReader(Array<Url *> *files, Package *pkg);

		// Owning package.
		Package *pkg;

		// Files to parse.
		Array<Url *> *files;

		// Get the syntax rules.
		virtual void STORM_FN readSyntaxRules();

		// Get the syntax options.
		virtual void STORM_FN readSyntaxOptions();

		// Get the types.
		virtual void STORM_FN readTypes();

		// Resolve types.
		virtual void STORM_FN resolveTypes();

		// Get all functions.
		virtual void STORM_FN readFunctions();
	};


	/**
	 * A reader for a single file. Use together with 'FilePkgReader'.
	 */
	class FileReader : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create a file reader.
		STORM_CTOR FileReader(Url *file, Package *into);

		// File.
		Url *file;

		// Package we're loading into.
		Package *pkg;

		// Get the syntax rules.
		virtual void STORM_FN readSyntaxRules();

		// Get the syntax options.
		virtual void STORM_FN readSyntaxOptions();

		// Get the types.
		virtual void STORM_FN readTypes();

		// Resolve types.
		virtual void STORM_FN resolveTypes();

		// Get all functions.
		virtual void STORM_FN readFunctions();
	};


	/**
	 * Specialization of PkgReader which uses a set of FileReaders. Provide a function which creates
	 * instances of a 'FileReader' to use.
	 */
	class FilePkgReader : public PkgReader {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR FilePkgReader(Array<Url *> *files, Package *pkg, Fn<FileReader *, Url *> *create);

		// Get the syntax rules.
		virtual void STORM_FN readSyntaxRules();

		// Get the syntax options.
		virtual void STORM_FN readSyntaxOptions();

		// Get the types.
		virtual void STORM_FN readTypes();

		// Resolve types.
		virtual void STORM_FN resolveTypes();

		// Get all functions.
		virtual void STORM_FN readFunctions();

	private:
		// Store all files in use.
		Array<FileReader *> *readers;

		// Create FileReaders.
		Fn<FileReader *, Url *> *create;

		// Populate 'readers' if it is not already done.
		void loadReaders();
	};


}
