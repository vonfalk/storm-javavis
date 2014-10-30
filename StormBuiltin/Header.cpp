#include "stdafx.h"
#include "Header.h"
#include "Exception.h"
#include "Parse.h"
#include "Utils/TextReader.h"
#include "Utils/FileStream.h"
using namespace util;

Header::Header(const Path &p) : file(p), parsed(false) {
	parse(); TODO("REMOVE!");
}

void Header::output(wostream &to) const {
	to << L"Header: " << file;
}

vector<Type> Header::getTypes() {
	parse();
	return types;
}

void Header::parse() {
	if (parsed)
		return;
	parsed = true;

	TextReader *reader = TextReader::create(new FileStream(file, FileStream::mRead));
	String contents = reader->getAll();
	delete reader;

	Tokenizer tok(contents, 0);
	try {
		parse(tok);
	} catch (const Error &e) {
		Error err(e.what(), file);
		throw err;
	}
}

void Header::parse(Tokenizer &tok) {
	String pkg;
	CppScope scope;

	while (tok.more()) {
		String token = tok.next();

		if (token[0] == '#') {
			// Ignore preprocessor text.
			if (token == L"#define")
				tok.next();
		} else if (token == L"STORM_FN") {
		} else if (token == L"STORM_CLASS") {
			if (!scope.isType())
				throw Error(L"STORM_CLASS only allowed in classes and structs!");
			Type t = { scope.name(), scope.super(), pkg, scope.cppName() };
			types.push_back(t);
		} else if (token == L"STORM_PKG") {
			pkg = parsePkg(tok);
		} else if (token == L"class" || token == L"struct") {
			String name = tok.next();
			if (tok.peek() != L";") {
				scope.push(true, name, findSuper(tok));
				tok.expect(L"{");
				PVAR(scope);
			}
		} else if (token == L"namespace") {
			scope.push(false, tok.next());
			tok.expect(L"{");
		} else if (token == L"template") {
			skipTemplate(tok);
		} else if (token == L"{") {
			scope.push();
		} else if (token == L"}") {
			scope.pop();
		}
	}
}
