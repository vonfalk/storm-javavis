#pragma once
#include "PkgReader.h"
#include "BSScope.h"
#include "Basic/BSContents.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * The main reader for the basic storm language.
		 */
		class Reader : public FilesReader {
			STORM_CLASS;
		public:
			// Ctor.
			STORM_CTOR Reader(Par<PkgFiles> files, Par<Package> pkg);

			virtual FileReader *STORM_FN createFile(Par<Url> path);
		};

		/**
		 * Single file representation.
		 */
		class File : public FileReader {
			STORM_CLASS;
		public:
			// Ctor.
			STORM_CTOR File(Par<Url> file, Par<Package> owner);

			// Dtor.
			~File();

			// Get types.
			void STORM_FN readTypes();

			// Update inheritance.
			void STORM_FN resolveTypes();

			// Get functions.
			void STORM_FN readFunctions();

			// The lookup.
			Auto<BSScope> scopeLookup;

			// All included packages, inside a scope.
			const Scope scope;

		private:
			// The file contents.
			Auto<Str> fileContents;

			// Position of the end of the use statements of 'fileContents'.
			Str::Iter headerEnd;

			// Contents.
			Auto<Contents> contents;

			// Read includes from the file.
			void readIncludes();

			// Find included packages.
			void setIncludes(Par<ArrayP<SrcName>> includes);

			// Set 'contents' to something good.
			void readContents();
		};
	}
}