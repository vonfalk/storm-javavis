#pragma once
#include "PkgReader.h"
#include "Package.h"
#include "Parse.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Reader for syntax files.
		 */
		class Reader : public FilesReader {
			STORM_CLASS;
		public:
			// Constructor.
			STORM_CTOR Reader(Par<PkgFiles> files, Par<Package> pkg);

			// Create a reader for a single file.
			virtual FileReader *STORM_FN createFile(Par<Url> path);
		};


		/**
		 * A file reader for syntax files.
		 */
		class File : public FileReader {
			STORM_CLASS;
		public:
			STORM_CTOR File(Par<Url> file, Par<Package> into);

			// Read syntax from the file.
			virtual void STORM_FN readSyntaxRules();
			virtual void STORM_FN readSyntaxOptions();

		private:
			// Contents from the file.
			Auto<Contents> contents;

			// Scope. Loaded when 'contents' is loaded.
			Scope scope;

			// Make sure 'contents' is loaded.
			void ensureLoaded();
		};

	}
}
