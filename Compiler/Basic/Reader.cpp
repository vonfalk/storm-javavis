#include "stdafx.h"
#include "Reader.h"
#include "Core/Io/Text.h"

namespace storm {
	namespace bs {

		static storm::FileReader *CODECALL createFile(Url *file, Package *pkg) {
			return new (file) FileReader(file, pkg);
		}

		PkgReader *reader(Array<Url *> *files, Package *pkg) {
			Engine &e = pkg->engine();
			return new (e) FilePkgReader(files, pkg, fnPtr(e, &createFile));
		}

		FileReader::FileReader(Url *file, Package *into) : storm::FileReader(file, into) {
			scopeLookup = new (this) BSLookup();
			scope = Scope(into, scopeLookup);
		}

		void FileReader::readTypes() {
			readContent();
		}

		void FileReader::resolveTypes() {
			readContent();
		}

		void FileReader::readFunctions() {
			readContent();
		}

		void FileReader::readContent() {
			if (content)
				return;

			Str *src = readAllText(file);
			Str::Iter i = readIncludes(src);
			readContent(src, i);
		}

		Str::Iter FileReader::readIncludes(Str *src) {
			TODO(L"FIXME!");
			return src->begin();
		}

		void FileReader::readContent(Str *src, Str::Iter start) {
			TODO(L"FIXME!");
		}

	}
}
