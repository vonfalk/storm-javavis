#include "stdafx.h"
#include "Type.h"
#include "Utils/Indent.h"
#include "Exception.h"
#include "World.h"

Type::Type(const CppName &name, const SrcPos &pos, bool valueType) :
	id(0), valueType(valueType), name(name), parent(L""), parentType(null), pos(pos) {}

void Type::add(const Variable &v) {
	for (nat i = 0; i < variables.size(); i++)
		if (variables[i].name == v.name)
			throw Error(L"Member variable " + toS(v.name) + L" already declared!", v.pos);

	variables.push_back(v);
}

void Type::resolveTypes(World &in) {
	if (!parent.empty())
		parentType = in.findType(parent, pos);

	for (nat i = 0; i < variables.size(); i++)
		variables[i].resolveTypes(in);
}

Size Type::size() const {
	Size s = Size::sPtr; // VTable.

	for (nat i = 0; i < variables.size(); i++)
		s += variables[i].type->size();

	return s;
}

vector<Offset> Type::ptrOffsets() const {
	Offset o = Offset::sPtr; // VTable.
	vector<Offset> r;

	for (nat i = 0; i < variables.size(); i++) {
		const Auto<CppType> &t = variables[i].type;
		if (isGcPtr(t))
			r.push_back(o);
		o += variables[i].type->size();
	}

	return r;
}

wostream &operator <<(wostream &to, const Type &type) {
	to << L"class " << type.name;
	if (!type.parent.empty())
		to << L" : " << type.parent;
	to << L" {\n";

	{
		Indent z(to);
		for (nat i = 0; i < type.variables.size(); i++)
			to << type.variables[i] << endl;
	}
	to << L"}";

	return to;
}
