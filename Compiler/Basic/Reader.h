#pragma once
#include "Compiler/Reader.h"

namespace storm {
	namespace basic {
		STORM_PKG(lang.bs);

		// Entry point for the syntax language.
		PkgReader *STORM_FN reader(Array<Url *> *files, Package *pkg);

		/**
		 * Main reader for Basic Storm.
		 */
		class FileReader : public storm::FileReader {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR FileReader(Url *file, Package *into);

			// Get types.
			void STORM_FN readTypes();

			// Update inheritance.
			void STORM_FN resolveTypes();

			// Get functions.
			void STORM_FN readFunctions();
		};

	}
}
