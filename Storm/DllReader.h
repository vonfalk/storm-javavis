#pragma once
#include "PkgReader.h"
#include "Shared/BuiltIn.h"

namespace storm {
	namespace dll {

		STORM_PKG(lang.dll);

		/**
		 * Load DLL:s at runtime.
		 */
		class Reader : public FilesReader {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Reader(Par<PkgFiles> files, Par<Package> pkg);

			// Create a file.
			virtual FileReader *STORM_FN createFile(Par<Url> path);
		};

		class DllReader : public FileReader {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR DllReader(Par<Url> file, Par<Package> into);

			// Syntax is not implemented yet, place it in a separate .bnf file!

			virtual void STORM_FN readTypes();
			virtual void STORM_FN resolveTypes();
			virtual void STORM_FN readFunctions();

		private:
			// Contents.
			const BuiltIn *contents;
		};
	}
}
