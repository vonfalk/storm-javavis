#include "stdafx.h"
#include "Type.h"
#include "Exception.h"

void Type::output(wostream &to) const {
	to << name << L"(" << package << L")";
}

String Type::fullName() const {
	if (package == L"")
		return name;
	else
		return package + L"." + name;
}

void Types::add(const Type &type) {
	types.insert(make_pair(type.cppName, type));
}

Type Types::find(const CppName &name, const CppName &scope) const {
	for (CppName current = scope; !current.empty(); current = current.parent()) {
		T::const_iterator i = types.find(current + name);
		if (i != types.end())
			return i->second;
	}

	// Fully qualified name?
	T::const_iterator i = types.find(name);
	if (i != types.end())
		return i->second;

	// Relative to some of the global "using namespace" declarations?
	for (nat z = 0; z < usedNamespaces.size(); z++) {
		T::const_iterator i = types.find(usedNamespaces[z] + name);
		if (i != types.end())
			return i->second;
	}

	throw Error(L"Type " + ::toS(name) + L" not found in " + ::toS(scope));
}

void Types::output(wostream &to) const {
	join(to, types, L"\n");
}

Types::Types(bool fc) : forCompiler(fc) {}

vector<Type> Types::getTypes() const {
	vector<Type> r;
	CppName n;
	n.parts.push_back(L"storm");
	n.parts.push_back(L"Type");
	if (types.count(n) == 0) {
		if (forCompiler)
			throw Error(L"Can not find the storm::Type type! This is needed for the compiler to work!");
	} else {
		r.push_back(types.find(n)->second);
	}

	for (T::const_iterator i = types.begin(); i != types.end(); ++i) {
		if (i->second.exported && i->first != n)
			r.push_back(i->second);
	}
	return r;
}

bool Types::external(const Type &t) const {
	return t.shared && !forCompiler;
}
