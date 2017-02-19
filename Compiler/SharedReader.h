#pragma once
#include "Compiler/Reader.h"
#include "Compiler/SharedLib.h"

namespace storm {
	namespace shared {
		// TODO: Place in another package for linux/unix. Alternatively, use a separate file extension
		// for both systems.
		STORM_PKG(lang.dll);

		// Entry point!
		PkgReader *STORM_FN reader(Array<Url *> *files, Package *pkg);

		/**
		 * Reader for shared libraries.
		 *
		 * We're not using file readers, as they assume we are reading text files.
		 */
		class SharedReader : public PkgReader {
			STORM_CLASS;
		public:
			STORM_CTOR SharedReader(Array<Url *> *files, Package *into);

			// Get the types.
			virtual void STORM_FN readTypes();

			// Resolve types.
			virtual void STORM_FN resolveTypes();

			// Get all functions.
			virtual void STORM_FN readFunctions();

		private:
			// All libraries queued for loading.
			Array<CppLoader> *toLoad;

			// Load all files if not already done.
			void load();
		};

	}
}
