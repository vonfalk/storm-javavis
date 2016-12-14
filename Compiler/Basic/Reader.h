#pragma once
#include "Compiler/Reader.h"
#include "Core/Str.h"
#include "Lookup.h"
#include "Content.h"

namespace storm {
	namespace bs {
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

			// The lookup.
			BSLookup *scopeLookup;

			// All included packages inside a scope.
			Scope scope;

		private:
			// Content.
			Content *content;

			// Read content from the file.
			void readContent();

			// Read includes from the file.
			Str::Iter readIncludes(Str *src);

			// Read the rest of the content.
			void readContent(Str *src, Str::Iter start);
		};

	}
}
