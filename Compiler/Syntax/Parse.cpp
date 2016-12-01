#include "stdafx.h"
#include "Parse.h"

namespace storm {
	namespace syntax {

		FileContents *parseSyntax(Url *file) {
			FileContents *r = new (file) FileContents();

			TODO(L"Parse the syntax in " << file);

			return r;
		}

	}
}
