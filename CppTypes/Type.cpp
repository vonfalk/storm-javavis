#include "stdafx.h"
#include "Type.h"
#include "Utils/Indent.h"
#include "Exception.h"
#include "World.h"

Type::Type(const CppName &name, const String &pkg, const SrcPos &pos) : id(0), name(name), pkg(pkg), pos(pos) {}

vector<Offset> Type::ptrOffsets() const {
	vector<Offset> r;
	ptrOffsets(r);
	return r;
}

wostream &operator <<(wostream &to, const Type &type) {
	type.print(to);
	return to;
}

/**
 * Class.
 */

Class::Class(const CppName &name, const String &pkg, const SrcPos &pos) :
	Type(name, pkg, pos), valueType(false), parent(L""), hiddenParent(false), parentType(null) {}

void Class::add(const Variable &v) {
	for (nat i = 0; i < variables.size(); i++)
		if (variables[i].name == v.name)
			throw Error(L"Member variable " + toS(v.name) + L" already declared!", v.pos);

	variables.push_back(v);
}

void Class::add(const Function &f) {
	// Don't bother checking for duplicates... That is done by the C++ compiler and Storm during boot.
	functions.push_back(f);
}

void Class::resolveTypes(World &in) {
	CppName ctx = name.parent();

	if (!parent.empty())
		parentType = in.types.find(parent, ctx, pos);

	for (nat i = 0; i < variables.size(); i++)
		variables[i].resolveTypes(in, ctx);

	for (nat i = 0; i < functions.size(); i++)
		functions[i].resolveTypes(in, ctx);
}

Size Class::size() const {
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

void Class::ptrOffsets(vector<Offset> &to) const {
	Size s;

	if (parentType) {
		parentType->ptrOffsets(to);
		s = parentType->size();
	} else if (!valueType) {
		s = Size::sPtr; // VTable.
	}

	for (nat i = 0; i < variables.size(); i++) {
		const Auto<TypeRef> &t = variables[i].type;
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

bool Class::hasDtor() const {
	// Always give destructors for value types.
	if (valueType)
		return true;

	// Ignore storm::Object.
	if (name == L"storm::Object")
		return false;

	for (nat i = 0; i < functions.size(); i++) {
		if (functions[i].name == Function::dtor)
			return true;
	}

	if (Class *c = as<Class>(parentType))
		return c->hasDtor();
	else
		return false;
}

void Class::print(wostream &to) const {
	to << L"class " << name;
	if (!parent.empty())
		to << L" : " << parent;
	to << L" {\n";

	{
		Indent z(to);
		for (nat i = 0; i < variables.size(); i++)
			to << variables[i] << endl;

		for (nat i = 0; i < functions.size(); i++)
			to << functions[i] << endl;
	}
	to << L"}";
}

/**
 * Enum.
 */

Enum::Enum(const CppName &name, const String &pkg, const SrcPos &pos) : Type(name, pkg, pos), bitmask(false) {}

void Enum::resolveTypes(World &world) {}

Size Enum::size() const {
	enum Test { foo = 0x10 };
	assert(Size::sInt.current() == sizeof(Test), L"Check the enum size on this machine!");
	// We might want to use sPtr or similar. I don't know the enum size on x86-64.
	return Size::sInt;
}

void Enum::ptrOffsets(vector<Offset> &append) const {}

void Enum::print(wostream &to) const {
	to << L"enum ";
	if (bitmask)
		to << L"bitmask ";
	to << name << L" {\n";
	{
		Indent z(to);
		for (nat i = 0; i < members.size(); i++)
			to << members[i] << L",\n";
	}
	to << L"}";
}

/**
 * Template.
 */

Template::Template(const CppName &name, const String &pkg, const CppName &generator, const SrcPos &pos) :
	name(name), pkg(pkg), generator(generator), pos(pos) {}