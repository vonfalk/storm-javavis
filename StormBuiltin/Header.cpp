#include "stdafx.h"
#include "Header.h"
#include "Exception.h"
#include "Parse.h"
#include "Function.h"
#include "Utils/TextReader.h"
#include "Utils/FileStream.h"
using namespace util;

Header::Header(const Path &p) : file(p), parsed(false) {}

void Header::output(wostream &to) const {
	to << L"Header: " << file;
}

const vector<Type> &Header::getTypes() {
	parse();
	return types;
}

const vector<Function> &Header::getFunctions() {
	parse();
	return functions;
}

void Header::parse() {
	if (parsed)
		return;
	parsed = true;

	String contents = readTextFile(file);
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
	CppType lastType;

	int depth = 0;

	while (tok.more()) {
		String token = tok.next();
		bool wasType = false;

		if (token[0] == '#') {
			// Ignore preprocessor text.
			if (token == L"#define")
				tok.next();
		} else if (token == L"STORM_FN") {
			functions.push_back(Function::read(pkg, scope, lastType, tok));
		} else if (token == L"STORM_CLASS" || token == L"STORM_VALUE") {
			if (!scope.isType())
				throw Error(L"STORM_CLASS or STORM_VALUE only allowed in classes and structs!");
			bool isValue = token == L"STORM_VALUE";

			Type t;
			if (scope.cppName().isObject()) {
				// storm::Object is the root object, ignore any
				// super-classes! They are just for convenience!
				t = Type(scope.name(), CppName(), pkg, scope.cppName(), isValue);
			} else {
				t = Type(scope.name(), scope.super(), pkg, scope.cppName(), isValue);
			}
			types.push_back(t);

			// Declare the destructor.
			functions.push_back(Function::dtor(pkg, scope));

			// Values need their destructor as well as copy and assignment operators.
			if (isValue) {
				functions.push_back(Function::copyCtor(pkg, scope));
				functions.push_back(Function::assignment(pkg, scope));
				// TODO: May need other functions as well.
			}
		} else if (token == L"STORM_PKG") {
			pkg = parsePkg(tok);
		} else if (token == L"STORM_CTOR") {
			Function ctor = Function::read(pkg, scope, CppType::tVoid(), tok);
			ctor.name = L"__ctor";
			functions.push_back(ctor);
		} else if (token == L"class" || token == L"struct") {
			String name = tok.next();
			if (tok.peek() == L"<")
				skipTemplate(tok);
			if (tok.peek() != L";") {
				scope.push(true, name, findSuper(tok));
				tok.expect(L"{");
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
		} else if (token == L";") {
		} else if (token == L"::") {
			if (!lastType.type.empty()) {
				lastType.type.parts.push_back(tok.next());
				wasType = true;
			}
		} else if (token == L"*") {
			if (!lastType.type.empty()) {
				wasType = true;
				lastType.isPtr = true;
			}
		} else if (token == L"&") {
			if (!lastType.type.empty()) {
				wasType = true;
				lastType.isRef = true;
			}
		} else if (token == L"const") {
			lastType.isConst = true;
			wasType = true;
		} else {
			if (!lastType.type.empty())
				lastType.clear();
			lastType.type.parts.push_back(token);
			wasType = true;
		}

		if (!wasType)
			lastType.clear();
	}
}
