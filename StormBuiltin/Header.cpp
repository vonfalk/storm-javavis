#include "stdafx.h"
#include "Header.h"
#include "Exception.h"
#include "Parse.h"
#include "Function.h"
#include "Utils/TextReader.h"
#include "Utils/FileStream.h"
using namespace util;

Header::Header(const Path &p, bool external) : file(p), external(external), parsed(false) {}

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

const vector<Variable> &Header::getVariables() {
	parse();
	return variables;
}

const vector<Thread> &Header::getThreads() {
	parse();
	return threads;
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

void checkEngineFn(const Function &fn) {
	if (fn.params.size() < 1)
		throw Error(L"The STORM_ENGINE_FN " + ::toS(fn) + L" must have at least one parameter.");

	const CppType &first = fn.params[0];
	bool fail = false;
	if (first.type.parts.size() < 1)
		fail = true;
	else if (first.type.parts.back() != L"EnginePtr")
		fail = true;
	else if (first.isPtr || first.isRef)
		fail = true;

	if (fail)
		throw Error(L"The first parameter of STORM_ENGINE_FN " + ::toS(fn) +
					L" must have a EnginePtr as the first parameter.");
}

void Header::addType(const CppScope &scope, const String &rawPkg, bool value) {
	if (!scope.isType())
		throw Error(L"STORM_CLASS or STORM_VALUE only allowed in classes and structs!");

	String pkg = rawPkg;
	CppScope tmpScope = scope;
	tmpScope.pop();
	while (tmpScope.isType()) {
		if (pkg == L"")
			pkg = tmpScope.name();
		else
			pkg += L"." + tmpScope.name();
		tmpScope.pop();
	}

	Type t(scope.name(), scope.super(), pkg, scope.cppName(), value, external);
	types.push_back(t);

	// Destructor.
	functions.push_back(Function::dtor(pkg, scope, external));

	// Values need their destructor as well as copy and assignment operators.
	if (value) {
		functions.push_back(Function::copyCtor(pkg, scope, external));
		functions.push_back(Function::assignment(pkg, scope, external));
		// TODO: May need other functions as well.
	}
}

void Header::parse(Tokenizer &tok) {
	String pkg;
	CppScope scope;
	CppType lastType;
	bool wasVirtual = false;

	int depth = 0;

	while (tok.more()) {
		String token = tok.next();
		bool wasType = false;
		FnFlags fnFlags = fnNone;
		if (wasVirtual)
			fnFlags |= fnVirtual;
		if (external)
			fnFlags |= fnExternal;


		if (token[0] == '#') {
			// Ignore preprocessor text.
			if (token == L"#define") {
				tok.next();
			}
		} else if (token == L"STORM_THREAD") {
			tok.expect(L"(");
			String name = tok.next();
			tok.expect(L")");
			tok.expect(L";");

			Thread t = {
				scope.cppName() + name,
				name,
				pkg
			};
			threads.push_back(t);
		} else if (token == L"STORM_FN") {
			functions.push_back(Function::read(fnFlags, pkg, scope, lastType, tok));
		} else if (token == L"STORM_SETTER") {
			// TODO: Check parameters.
			functions.push_back(Function::read(fnFlags | fnSetter, pkg, scope, lastType, tok));
		} else if (token == L"STORM_VAR") {
			variables.push_back(Variable::read(external, scope, tok));
		} else if (token == L"STORM_ENGINE_FN") {
			functions.push_back(Function::read(fnFlags | fnEngine, pkg, scope, lastType, tok));
			checkEngineFn(functions.back());
		} else if (token == L"STORM_CLASS") {
			addType(scope, pkg, false);
		} else if (token == L"STORM_VALUE") {
			addType(scope, pkg, true);
		} else if (token == L"STORM_PKG") {
			pkg = parsePkg(tok);
		} else if (token == L"STORM_CTOR") {
			fnFlags &= ~fnVirtual;
			Function ctor = Function::read(fnFlags, pkg, scope, CppType::tVoid(), tok);
			ctor.name = L"__ctor";
			functions.push_back(ctor);
		} else if (token == L"STORM_CAST_CTOR") {
			fnFlags &= ~fnVirtual;
			Function ctor = Function::read(fnFlags | fnCastCtor, pkg, scope, CppType::tVoid(), tok);
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
		} else if (token == L"using") {
			// Consume 'namespace' to avoid misinterpretation.
			tok.next();
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
			if (!lastType.isEmpty()) {
				lastType.type.parts.push_back(tok.next());
				wasType = true;
			}
		} else if ((token == L"Array" || token == L"ArrayP") && tok.peek() == L"<") {
			tok.next();

			lastType = CppType::read(tok);
			if (token == L"Array")
				lastType.isArray = true;
			else
				lastType.isArrayP = true;

			tok.expect(L">");

			wasType = true;
		} else if (token == L"FnPtr" && tok.peek() == L"<") {
			tok.next();
			lastType.clear();
			lastType.isFnPtr = true;
			lastType.fnParams.push_back(CppType::read(tok));
			while (tok.next() == L",")
				lastType.fnParams.push_back(CppType::read(tok));

			wasType = true;
		} else if (token == L"MAYBE" && tok.peek() == L"(") {
			tok.next();
			lastType = CppType::read(tok);
			lastType.isMaybe = true;
			wasType = true;
			tok.expect(L")");
		} else if (token == L"*") {
			if (!lastType.isEmpty()) {
				wasType = true;
				lastType.isPtr = true;
			}
		} else if (token == L"&") {
			if (!lastType.isEmpty()) {
				wasType = true;
				lastType.isRef = true;
			}
		} else if (token == L"const") {
			lastType.isConst = true;
			wasType = true;
		} else if (token == L"virtual") {
			wasVirtual = true;
			wasType = true;
		} else {
			if (!lastType.isEmpty())
				lastType.clear();
			lastType.type.parts.push_back(token);
			wasType = true;
		}

		if (!wasType) {
			lastType.clear();
			wasVirtual = false;
		}
	}
}
