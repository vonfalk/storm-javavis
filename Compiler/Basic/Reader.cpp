#include "stdafx.h"
#include "Reader.h"

namespace storm {
	namespace basic {

		static storm::FileReader *CODECALL createFile(Url *file, Package *pkg) {
			return new (file) FileReader(file, pkg);
		}

		PkgReader *reader(Array<Url *> *files, Package *pkg) {
			Engine &e = pkg->engine();
			return new (e) FilePkgReader(files, pkg, fnPtr(e, &createFile));
		}

		FileReader::FileReader(Url *file, Package *into) : storm::FileReader(file, into) {}

		void FileReader::readTypes() {}

		void FileReader::readTypes() {}

		void FileReader::readFunctions() {}

	}
}
