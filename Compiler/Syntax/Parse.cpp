#include "stdafx.h"
#include "Parse.h"
#include "Tokenizer.h"
#include "Core/Io/Text.h"
#include "Core/Str.h"

namespace storm {
	namespace syntax {

		static FileContents *parseFile(Engine &e, Tokenizer &src) {
			FileContents *r = new (e) FileContents();

			TODO(L"Parse the file!");

			return r;
		}

		FileContents *parseSyntax(Url *file) {
			Str *data = readAllText(file);
			Tokenizer src(file, data, 0);
			return parseFile(file->engine(), src);
		}

	}
}
