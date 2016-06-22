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
	CppName ctx = name.parent();

	if (!parent.empty())
		parentType = in.findType(parent, ctx, pos);

	for (nat i = 0; i < variables.size(); i++)
		variables[i].resolveTypes(in, ctx);
}

Size Type::size() const {
	Size s;

	if (parentType) {
		s = parentType->size();
	} else if (!valueType) {
		s = Size::sPtr; // VTable.
	}

	for (nat i = 0; i < variables.size(); i++)
		s += variables[i].type->size();

	return s;
}

vector<Offset> Type::ptrOffsets() const {
	vector<Offset> r;
	ptrOffsets(r);
	return r;
}

void Type::ptrOffsets(vector<Offset> &to) const {
	Size s;

	if (parentType) {
		parentType->ptrOffsets(to);
		s = parentType->size();
	} else if (!valueType) {
		s = Size::sPtr; // VTable.
	}

	for (nat i = 0; i < variables.size(); i++) {
		const Auto<CppType> &t = variables[i].type;
		Size size = t->size();

		// Make sure to properly align 's'.
		s += size.align();

		if (isGcPtr(t)) {
			// TODO? Export all pointers, regardless if they are to a GC:d object or not?
			to.push_back(Offset(s));
		} else if (Auto<ResolvedType> res = t.as<ResolvedType>()) {
			// Inline this type into ourselves.
			vector<Offset> o = res->type->ptrOffsets();
			for (nat i = 0; i < o.size(); i++)
				to.push_back(o[i] + s);
		}

		s += size;
	}
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
