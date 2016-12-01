#include "stdafx.h"
#include "Reader.h"

namespace storm {
	namespace syntax {

		static storm::FileReader *CODECALL createFile(Url *file, Package *pkg) {
			return new (file) FileReader(file, pkg);
		}

		PkgReader *reader(Array<Url *> *files, Package *pkg) {
			Engine &e = pkg->engine();
			return new (e) FilePkgReader(files, pkg, fnPtr(e, &createFile));
		}


		FileReader::FileReader(Url *file, Package *into) : storm::FileReader(file, into), c(null) {}

		void FileReader::readSyntaxRules() {
			ensureLoaded();
		}

		void FileReader::readSyntaxProductions() {
			ensureLoaded();
		}

		void FileReader::ensureLoaded() {
			if (c)
				return;

			c = parseSyntax(file);
		}

	}
}
