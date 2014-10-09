#include "stdafx.h"
#include "Function.h"
#include "Tokenizer.h"
#include "Exception.h"
#include "Utils/FileStream.h"
#include "Utils/TextReader.h"

namespace stormbuiltin {
	using namespace util;

	wostream &operator <<(wostream &to, const Function &fn) {
		to << fn.result << L" " << fn.name << L"(";
		join(to, fn.params, L", ");
		to << L"), " << fn.cppName << L", in " << fn.header;
		return to;
	}

	String findPackage(Tokenizer &tok) {
		if (tok.next() != L"(")
			throw Error(L"Expected (");
		String pkg = tok.next();
		for (String tmp = tok.next(); tmp != L")"; tmp = tok.next()) {
			pkg += tmp;
		}
		return pkg;
	}

	Function findFunction(String &returnType, Tokenizer &tok) {
		Function fn;
		fn.result = returnType;
		fn.name = tok.next();
		if (tok.next() != L"(")
			throw Error(L"Expected (");

		String param;
		bool inName = false;
		while (true) {
			String token = tok.next();
			if (token == L")") {
				if (param != L"")
					fn.params.push_back(param);
				break;
			}

			if (token == L"const") {
			} else if (token == L"*") {
			} else if (token == L"&") {
			} else if (token == L",") {
				fn.params.push_back(param);
				param = L"";
				inName = false;
			} else if (!inName) {
				if (param.endsWith(L"::") || param == L"") {
					param += token;
				} else if (token == L"::") {
					param += token;
				} else {
					inName = true;
				}
			}
		}

		return fn;
	}

	vector<Function> parseFile(Tokenizer &tok) {
		vector<Function> found;
		String package;
		vector<String> scope;
		vector<nat> extra;

		String lastType;

		while (tok.more()) {
			bool wasType = false;
			String token = tok.next();

			if (token == L"STORM_PKG") {
				package = findPackage(tok);
			} else if (token == L"STORM_FN") {
				if (lastType != L"#define") {
					Function fn = findFunction(lastType, tok);
					fn.package = package;
					vector<String> t = scope;
					t.push_back(fn.name);
					fn.cppName = join(t, L"::");
					found.push_back(fn);
				}
			} else if (token == L"class" || token == L"struct") {
				scope.push_back(tok.next());
				extra.push_back(0);
				if (tok.peek() != L";")
					while (tok.next() != L"{");
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
			} else if (token == L";") {
			} else {
				if (token == L"*" || token == L"&") {
					wasType = true;
				} else if (lastType.endsWith(L"::") || lastType == L"") {
					wasType = true;
					lastType += token;
				} else if (token == L"::") {
					wasType = true;
					lastType += token;
				}
			}

			if (!wasType)
				lastType = L"";
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
			result = parseFile(tok);
			for (nat i = 0; i < result.size(); i++) {
				result[i].header = file;
			}
			return result;
		} catch (const Error &e) {
			Error err(e.what(), file);
			throw err;
		}
	}

}
