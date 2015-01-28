#include "stdafx.h"
#include "Output.h"
#include "Exception.h"

/**
 * Helpers
 */

void vtableSymbolName(wostream &to, const CppName &name) {
	to << L"??_7";
	for (nat i = name.parts.size(); i > 0; i--) {
		to << name.parts[i-1] << L"@";
	}
	to << L"@6B@";
}

String vtableFnName(const CppName &name) {
	String cppName = toS(name);
	for (nat i = 0; i < cppName.size(); i++)
		if (cppName[i] == ':')
			cppName[i] = '_';
	return L"cppVTable_" + cppName;
}

void fnPtr(wostream &to, const Function &fn, const Types &types) {
	CppName scope = fn.cppScope.scopeName();

	to << fn.result.fullName(types, scope) << "(CODECALL ";
	if (fn.cppScope.isType())
		to << fn.cppScope.cppName() << L"::";

	to << L"*)(";
	for (nat i = 0; i < fn.params.size(); i++) {
		if (i != 0)
			to << L", ";
		to << fn.params[i].fullName(types, scope);
	}
	to << L")";
	if (fn.isConst)
		to << L" const";
}

String fixPath(String str) {
	for (nat i = 0; i < str.size(); i++)
		if (str[i] == '\\')
			str[i] = '/';
	return str;
}

/**
 * Generation
 */

String typeList(const Types &types) {
	std::wostringstream out;
	vector<Type> t = types.getTypes();
	for (nat i = 0; i < t.size(); i++) {
		Type &type = t[i];

		out << L"{ Name(L\"" << type.package << L"\"), ";
		out << L"L\"" << type.name << L"\", ";

		if (type.super.empty()) {
			out << L"Name(), ";
		} else {
			Type super = types.find(type.super, type.cppName.parent());
			out << L"Name(L\"" << super.fullName() << L"\"), ";
		}

		out << L"sizeof(" << type.cppName << L"), ";

		if (type.value) {
			out << L"typeValue, ";
		} else {
			out << L"typeClass, ";
		}

		out << i << L", ";
		if (type.value) {
			out << L"null ";
		} else {
			out << type.cppName << L"::cppVTable() ";
		}
		out << L"},\n";
	}

	return out.str();
}

String typeFunctions(const Types &types) {
	std::wostringstream out;
	vector<Type> t = types.getTypes();
	for (nat i = 0; i < t.size(); i++) {
		Type &type = t[i];
		String fn = vtableFnName(type.cppName);

		out << L"storm::Type *" << type.cppName << L"::type(Engine &e) { return e.builtIn(" << i << L"); }\n";
		out << L"storm::Type *" << type.cppName << L"::type(Object *o) { return type(o->myType->engine); }\n";
		if (!type.value) {
			// Values do not have type information.
			out << L"extern \"C\" void *" << fn << L"();\n";
			out << L"void *" << type.cppName << L"::cppVTable() { return " << fn << L"(); }\n";
		}
	}

	return out.str();
}

String vtableCode(const Types &types) {
	std::wostringstream out;
	vector<Type> t = types.getTypes();

	out << L".386\n";
	out << L".model flat, c\n\n";
	out << L".data\n\n";

	for (nat i = 0; i < t.size(); i++) {
		Type &type = t[i];
		if (type.value)
			continue;

		vtableSymbolName(out, type.cppName);
		out << L" proto syscall\n";
		out << vtableFnName(type.cppName) << L" proto\n\n";
	}

	out << L"\n.code\n\n";

	for (nat i = 0; i < t.size(); i++) {
		Type &type = t[i];
		if (type.value)
			continue;

		String fn = vtableFnName(type.cppName);

		out << fn << L" proc\n";
		out << L"\tmov eax, "; vtableSymbolName(out, type.cppName); out << L"\n";
		out << L"\tret\n";
		out << fn << L" endp\n\n";
	}

	out << L"end\n";

	return out.str();
}

void functionList(wostream &out, const vector<Function> &fns, const Types &types) {
	for (nat i = 0; i < fns.size(); i++) {
		const Function &fn = fns[i];
		CppName scope = fn.cppScope.scopeName();

		// Package
		out << L"{ Name(L\"" << fn.package << L"\"), ";

		// Member of?
		if (fn.cppScope.isType()) {
			Type member = types.find(fn.cppScope.cppName(), scope);
			out << L"L\"" << member.name << L"\", ";
		} else {
			out << "null, ";
		}

		// Result
		if (fn.result.isVoid()) {
			out << L"Name(), ";
		} else {
			out << L"Name(L\"" << types.find(fn.result.type, scope).fullName() << L"\"), ";
		}

		// Name
		out << L"L\"" << fn.name << L"\", ";

		// Params
		out << L"list(" << fn.params.size();
		for (nat i = 0; i < fn.params.size(); i++) {
			out << L", Name(L\"" << types.find(fn.params[i].type, scope).fullName() << L"\")";
		}
		out << L"), ";

		CppName name = fn.cppScope.cppName() + CppName(vector<String>(1, fn.name));

		// Function ptr.
		if (fn.name == L"__ctor") {
			out << L"address(&create" << (fn.params.size() + 1) << L"<";
			out << fn.cppScope.cppName();
			for (nat i = 0; i < fn.params.size(); i++) {
				CppType t = fn.params[i].fullName(types, scope);
				if (t.isPar) {
					// Efficiency hack: pass by ptr to the create-fn,
					// it will be casted to Auto by C++ later and do
					// the right thing in all cases anyway.
					t.isPar = false;
					t.isPtr = true;
				}
				out << L", " << t;
			}
			out << L">)";
		} else if (fn.name == L"__dtor") {
			out << L"address(&destroy<" << fn.cppScope.cppName() << L">)";
		} else {
			out << L"address<";
			fnPtr(out, fn, types);
			out << L">(&" << name << L")";
		}
		out << " },\n";
	}
}

String functionList(const vector<Header *> &headers, const Types &types) {
	std::wostringstream out;

	for (nat i = 0; i < headers.size(); i++) {
		Header &header = *headers[i];
		const vector<Function> &fns = header.getFunctions();

		try {
			functionList(out, fns, types);
		} catch (const Error &e) {
			Error err(e.what(), header.file);
			throw err;
		}
	}

	return out.str();
}

String headerList(const vector<Header *> &headers, const Path &root) {
	std::wostringstream out;

	for (nat i = 0; i < headers.size(); i++) {
		Header &header = *headers[i];

		if (header.getFunctions().size() > 0 || header.getTypes().size() > 0) {
			Path rel = header.file.makeRelative(root);
			out << L"#include \"" << fixPath(rel.toS()) << L"\"\n";
		}
	}

	return out.str();
}
