#include "stdafx.h"
#include "Function.h"
#include "Tokenizer.h"
#include "Exception.h"
#include "Utils/FileStream.h"
#include "Utils/TextReader.h"

namespace stormbuiltin {
	using namespace util;

	String findPackage(Tokenizer &tok) {
		if (tok.next() != L"(")
			throw Error(L"Expected (");
		String pkg = tok.next();
		for (String tmp = tok.next(); tmp != L")"; tmp = tok.next()) {
			pkg += tmp;
		}
		return pkg;
	}

	vector<Function> parseFile(Tokenizer &tok) {
		vector<Function> found;
		String package;
		vector<String> scope;
		vector<nat> extra;

		while (tok.more()) {
			String token = tok.next();

			if (token == L"STORM_PKG") {
				package = findPackage(tok);
			} else if (token == L"namespace") {
				scope.push_back(tok.next());
				extra.push_back(0);
				if (tok.next() != L"{")
					throw Error(L"Expected {");
			} else if (token == L"{") {
				if (extra.size() > 0)
					extra.back()++;
			} else if (token == L"}") {
				if (extra.size() > 0) {
					if (extra.back() == 0) {
						extra.pop_back();
						scope.pop_back();
					} else {
						extra.back()--;
					}
				}
			}
		}

		return found;
	}

	vector<Function> parseFile(const Path &file) {
		vector<Function> result;
		TextReader *reader = TextReader::create(new FileStream(file, FileStream::mRead));
		String contents = reader->getAll();
		delete reader;

		Tokenizer tok(file, contents, 0);
		try {
			return parseFile(tok);
		} catch (const Error &e) {
			Error err(e.what(), file);
			throw err;
		}
	}

}
