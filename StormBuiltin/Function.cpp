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

	struct Scope {
		String name;
		nat extra;
		bool isType;
	};

	String toS(const Scope &s) {
		return s.name;
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

	Function findFunction(Tokenizer &tok) {
		Function fn;
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

	String creationStr(const Function &fn, const vector<Scope> &scope) {
		std::wostringstream to;
		to << L"create" << fn.params.size();
		to << L"<";
		join(to, scope, L"::");
		for (nat i = 1; i < fn.params.size(); i++) {
			to << L", " << fn.params[i];
		}
		to << L">";
		return to.str();
	}

	Function parseFn(Tokenizer &tok, const vector<Scope> &scope, const String &retType, const String &package) {
		Function fn = findFunction(tok);
		fn.result = retType;
		fn.package = package;
		if (!scope.empty() && scope.back().isType)
			fn.classMember = scope.back().name;
		fn.cppName = join(scope, L"::");
		if (!fn.cppName.empty())
			fn.cppName += L"::";
		fn.cppName += fn.name;
		return fn;
	}

	String parseType(Tokenizer &tok) {
		std::wostringstream result(tok.next());

		while (tok.peek() == L"::") {
			result << tok.next();
			result << tok.next();
		}

		return result.str();
	}

	String parseSuper(Tokenizer &tok) {
		if (tok.peek() == L";")
			return L"";

		String result;
		nat state = 0;
		while (tok.peek() != L"{") {
			String t = tok.next();
			switch (state) {
			case 0:
				if (t == L"," || t == L":")
					state = 1;
				break;
			case 1:
				if (t == L"public" && result == L"")
					result = parseType(tok);
				state = 1;
				break;
			}
		}

		tok.next();
		if (result == L"Object")
			return L"";
		return result;
	}

	File parseFile(Tokenizer &tok) {
		File r;
		vector<Function> &found = r.fns;
		vector<Type> &types = r.types;

		String package;
		vector<Scope> scope;

		String lastType;
		bool addType = false;

		while (tok.more()) {
			bool wasType = false;
			String token = tok.next();

			if (token == L"#define") {
				tok.next();
			} else if (token == L"STORM_PKG") {
				package = findPackage(tok);
			} else if (token == L"STORM_FN") {
				Function fn = parseFn(tok, scope, lastType, package);
				found.push_back(fn);
			} else if (token == L"STORM") {
				String next = tok.peek();
				if (next == L"class" || next == L"struct") {
					addType = true;
				} else {
					Function fn = parseFn(tok, scope, L"", package);
					fn.result = fn.name;
					fn.name = L"__ctor";
					if (scope.empty() || !scope.back().isType)
						throw Error(L"Constructors must live in types.");
					fn.cppName = creationStr(fn, scope);
					found.push_back(fn);
				}
			} else if (token == L"class" || token == L"struct") {
				Scope s = { tok.next(), 0, true };
				scope.push_back(s);
				String super = parseSuper(tok);

				if (addType) {
					Type c = { s.name, super, package };
					types.push_back(c);
				}
			} else if (token == L"namespace") {
				Scope s = { tok.next(), 0, false };
				scope.push_back(s);
				if (tok.next() != L"{")
					throw Error(L"Expected {");
			} else if (token == L"{") {
				addType = false;
				if (!scope.empty())
					scope.back().extra++;
			} else if (token == L"}") {
				addType = false;
				if (!scope.empty()) {
					if (scope.back().extra == 0)
						scope.pop_back();
					else
						scope.back().extra--;
				}
			} else if (token == L";") {
			} else if (token.endsWith(L":")) {
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
				addType = false;
			}

			if (!wasType)
				lastType = L"";
		}

		return r;
	}

	File parseFile(const Path &file) {
		File result;
		TextReader *reader = TextReader::create(new FileStream(file, FileStream::mRead));
		String contents = reader->getAll();
		delete reader;

		Tokenizer tok(file, contents, 0);
		try {
			result = parseFile(tok);
			for (nat i = 0; i < result.fns.size(); i++) {
				result.fns[i].header = file;
			}
			return result;
		} catch (const Error &e) {
			Error err(e.what(), file);
			throw err;
		}
	}


	void File::add(const File &o) {
		fns.insert(fns.end(), o.fns.begin(), o.fns.end());
		types.insert(types.end(), o.types.begin(), o.types.end());
	}

}
