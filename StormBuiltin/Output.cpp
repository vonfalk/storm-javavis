#include "stdafx.h"
#include "Output.h"


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
