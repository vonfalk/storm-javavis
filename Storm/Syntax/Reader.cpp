#include "stdafx.h"
#include "Reader.h"
#include "Parse.h"

namespace storm {
	namespace syntax {

		Reader::Reader(Par<PkgFiles> files, Par<Package> pkg) :
			FilesReader(files, pkg) {}

		FileReader *Reader::createFile(Par<Url> url) {
			return CREATE(File, this, url, pkg);
		}

		File::File(Par<Url> file, Par<Package> into) :
			FileReader(file, into) {

			PLN("Reading file: " << file);
			// TODO: This should be done later.
			Auto<Contents> r = parseSyntax(file);
			PVAR(r);
		}

		void File::readSyntax() {
			// TODO!
		}

	}
}
