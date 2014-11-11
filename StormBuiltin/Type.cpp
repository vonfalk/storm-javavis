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

	throw Error(L"Type " + ::toS(name) + L" not found in " + ::toS(scope));
}

void Types::output(wostream &to) const {
	join(to, types, L"\n");
}

vector<Type> Types::getTypes() const {
	vector<Type> r;
	for (T::const_iterator i = types.begin(); i != types.end(); ++i) {
		if (i->second.exported)
			r.push_back(i->second);
	}
	return r;
}
