#pragma once
#include "PkgReader.h"
#include "SyntaxSet.h"

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
			STORM_CTOR Reader(Auto<PkgFiles> files, Auto<Package> pkg);

		protected:
			virtual FileReader *createFile(const Path &path);
		};

		/**
		 * Single file representation.
		 */
		class File : public FileReader {
			STORM_CLASS;
		public:
			// Ctor.
			File(const Path &file, Auto<Package> owner);

			// Dtor.
			~File();

			// Get types.
			void readTypes();

		private:
			// The file contents.
			String contents;

			// Length of header. The header is the use statements.
			nat headerSize;

			// Syntax to use while parsing.
			SyntaxSet syntax;

			// All included packages.
			vector<Package *> includes;

			// Read includes from the file.
			void readIncludes();

			// Find included packages.
			void setIncludes(const vector<Name> &includes);
		};
	}
}
