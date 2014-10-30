#include "stdafx.h"
#include "Output.h"

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


/**
 * Generation
 */


String typeList(const Types &types) {
	std::wostringstream out;
	vector<Type> t = types.getTypes();
	for (nat i = 0; i < t.size(); i++) {
		Type &type = t[i];

		out << L"{ Name(L\"" << type.package << L"\"), ";
		out << L"\"" << type.name << L"\", ";

		if (type.super.empty()) {
			out << L"Name(), ";
		} else {
			Type super = types.find(type.super, type.cppName);
			out << L"Name(L\"" << super.fullName() << L"\"), ";
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
		out << L"extern \"C\" void *" << fn << L"();\n";
		out << L"void *" << type.cppName << L"::cppVTable() { return " << fn << L"(); }\n";
	}

	return out.str();
}

String vtableCode(const Types &types) {
	std::wostringstream out;
	vector<Type> t = types.getTypes();

	for (nat i = 0; i < t.size(); i++) {
		Type &type = t[i];

		vtableSymbolName(out, type.cppName);
		out << L" proto syscall\n";
		out << vtableFnName(type.cppName) << L" proto\n\n";
	}

	for (nat i = 0; i < t.size(); i++) {
		Type &type = t[i];
		String fn = vtableFnName(type.cppName);

		out << fn << L" proc\n";
		out << L"\tmov eax, "; vtableSymbolName(out, type.cppName); out << L"\n";
		out << L"\tret\n";
		out << fn << L" endp\n\n";
	}

	out << L"end\n";

	return out.str();
}
