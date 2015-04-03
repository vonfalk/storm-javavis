#pragma once

#include "SyntaxRule.h"
#include "PkgReader.h"

namespace storm {

	// Check if the path is a bnf file.
	bool isBnfFile(Par<Url> file);

	// Read syntax definitions from the given file, and merge them
	// into the given hash map. Throws an appropriate exception on
	// failure. Note that no ownership of the SyntaxRule is taken.
	void parseBnf(SyntaxRules &to, Par<Url> file, const Scope &scope);


	namespace bnf {
		STORM_PKG(lang.bnf);

		/**
		 * BNF reader class.
		 */
		class Reader : public PkgReader {
			STORM_CLASS;
		public:
			// Create
			STORM_CTOR Reader(Par<PkgFiles> files, Par<Package> pkg);

			// Read files.
			virtual void readSyntax(SyntaxRules &to);
		};
	}
}
