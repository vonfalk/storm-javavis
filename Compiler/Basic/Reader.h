#pragma once
#include "Compiler/Reader.h"
#include "Compiler/FileReader.h"
#include "Core/Str.h"
#include "Lookup.h"
#include "Content.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		// Entry point for the syntax language.
		PkgReader *STORM_FN reader(Array<Url *> *files, Package *pkg);

		/**
		 * First stage reader for Basic Storm. At this stage we read the use statements to figure
		 * out which syntax to include in the rest of the file.
		 */
		class UseReader : public FileReader {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR UseReader(FileInfo *info);

			// No types or anything in here.

		protected:
			// Create the next part.
			virtual MAYBE(FileReader *) STORM_FN createNext();
		};

		/**
		 * Second stage reader for Basic Storm. Here, we receive all includes from the previous
		 * state and read the rest of the file.
		 */
		class CodeReader : public FileReader {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR CodeReader(FileInfo *info, Array<SrcName *> *includes);

			// Get types.
			void STORM_FN readTypes();

			// Update inheritance.
			void STORM_FN resolveTypes();

			// Get functions.
			void STORM_FN readFunctions();

			// Current scope.
			Scope scope;

		private:
			// Content.
			Content *content;

			// Read content from the file.
			void readContent();
		};

	}
}
