#include "stdafx.h"
#include "Parse.h"

namespace storm {
	namespace syntax {

		static FileContents *parseFile(Engine &e) {
			FileContents *r = new (e) FileContents();

			return r;
		}

		FileContents *parseSyntax(Url *file) {
			TODO(L"Parse the syntax in " << file);

			return parseFile(file->engine());
		}

	}
}
