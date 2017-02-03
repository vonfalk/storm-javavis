#pragma once
#include "Thread.h"
#include "Package.h"
#include "Core/Io/Url.h"
#include "Syntax/Parser.h"

namespace storm {
	STORM_PKG(lang);

	/**
	 * Information about a single file to be read.
	 */
	class FileInfo : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create, read contents from the file.
		STORM_CTOR FileInfo(Url *file, Package *pkg);

		// Create, provide contents and a starting point.
		STORM_CTOR FileInfo(Str *contents, Str::Iter start, Url *file, Package *pkg);

		// Contents of this file.
		Str *contents;

		// Start point.
		Str::Iter start;

		// Url to the file.
		Url *url;

		// Package containing the file.
		Package *pkg;

		// Generate a file info for continuing from 'pos'.
		FileInfo *STORM_FN next(Str::Iter pos);
	};

	/**
	 * A reader for a part of a single file. Use together with 'FilePkgReader'.
	 *
	 * This interface supports the reading of a file to be split into multiple parts. Each file may
	 * provide a new FileReader when 'next' is called. If so, that reader is used to get more data
	 * from the file.
	 */
	class FileReader : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create a file reader.
		STORM_CTOR FileReader(FileInfo *info);

		// File content.
		FileInfo *info;

		// Get the next part of this file.
		MAYBE(FileReader *) STORM_FN next();

		// Get the syntax rules.
		virtual void STORM_FN readSyntaxRules();

		// Get the syntax options.
		virtual void STORM_FN readSyntaxProductions();

		// Get the types.
		virtual void STORM_FN readTypes();

		// Resolve types.
		virtual void STORM_FN resolveTypes();

		// Get all functions.
		virtual void STORM_FN readFunctions();

		/**
		 * For language server integration. Overload either 'rootRule' or 'createParser'.
		 */

		// Get the initial rule used for parsing this language.
		virtual syntax::Rule *STORM_FN rootRule();

		// Create a parser for this language.
		virtual syntax::InfoParser *STORM_FN createParser();

	protected:
		// Create any additional file readers. Only called once.
		virtual MAYBE(FileReader *) STORM_FN createNext();

	private:
		// The previously created next part.
		FileReader * nextPart;
	};

}
