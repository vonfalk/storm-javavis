#include "stdafx.h"
#include "Type.h"
#include "Exception.h"

Types::Types(bool fc) : forCompiler(fc), cacheValid(false) {}

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
	invalidate();
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

void Types::invalidate() const {
	cacheValid = false;
}

void Types::create() const {
	if (cacheValid)
		return;

	ordered.clear();

	CppName n;
	n.parts.push_back(L"storm");
	n.parts.push_back(L"Type");
	if (types.count(n) == 0) {
		if (forCompiler)
			throw Error(L"Can not find the storm::Type type! This is needed for the compiler to work!");
	} else {
		ordered.push_back(types.find(n)->second);
	}

	for (T::const_iterator i = types.begin(); i != types.end(); ++i) {
		if (i->second.exported && i->first != n)
			ordered.push_back(i->second);
	}

	typeIds.clear();
	for (nat i = 0; i < ordered.size(); i++) {
		typeIds.insert(make_pair(ordered[i].cppName, i));
	}

	cacheValid = true;
}

const vector<Type> &Types::getTypes() const {
	create();
	return ordered;
}

nat Types::typeId(const CppName &name) const {
	create();

	Tid::const_iterator i = typeIds.find(name);
	if (i == typeIds.end()) {
		throw Error(L"Could not find " + ::toS(name) + L" in the list of types!");
	} else {
		return i->second;
	}
}

bool Types::external(const Type &t) const {
	return t.shared && !forCompiler;
}
