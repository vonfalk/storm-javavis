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

vector<ScannedVar> Type::scannedVars() const {
	vector<ScannedVar> r;
	scannedVars(r);
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
	Type(name, pkg, pos), valueType(false), parent(L""), hiddenParent(false),
	dtorFound(false), parentType(null), threadType(null) {}

void Class::resolveTypes(World &in) {
	CppName ctx = name;

	if (!parent.empty() && !hiddenParent)
		parentType = in.types.find(parent, ctx, pos);

	if (!thread.empty())
		threadType = in.threads.find(thread, ctx, pos);

	for (nat i = 0; i < variables.size(); i++)
		variables[i].resolveTypes(in, ctx);
}

Size Class::size() const {
	Size s;

	if (parentType) {
		s = parentType->size();
		s += s.align(); // align to its natural alignment.
	} else if (!valueType) {
		s = Size::sPtr; // VTable.
	}

	for (nat i = 0; i < variables.size(); i++) {
		Size t = variables[i].type->size();
		t += t.align(); // align to its natural alignment.
		s += t;
	}

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

void Class::scannedVars(vector<ScannedVar> &append) const {
	if (parentType)
		parentType->scannedVars(append);

	for (nat i = 0; i < variables.size(); i++) {
		const Auto<TypeRef> &t = variables[i].type;

		if (isGcPtr(t)) {
			if (t.as<RefType>()) {
				ScannedVar v = { name, L"" };
				append.push_back(v);
			} else {
				ScannedVar v = { name, variables[i].name };
				append.push_back(v);
			}
		} else if (Auto<ResolvedType> res = t.as<ResolvedType>()) {
			// Inline this type into ourselves.
			vector<ScannedVar> o = res->type->scannedVars();
			for (nat j = 0; j < o.size(); j++) {
				ScannedVar v = { name, String(variables[i].name) + L"." + o[j].varName };
				append.push_back(v);
			}
		}
	}
}


bool Class::hasDtor() const {
	// Always give destructors for value types.
	if (valueType)
		return true;

	// Ignore storm::Object.
	if (name == L"storm::Object")
		return false;

	if (dtorFound)
		return true;

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
	}
	to << L"}";
}

/**
 * Class namespace.
 */

ClassNamespace::ClassNamespace(World &world, Class &owner) : world(world), owner(owner) {}

void ClassNamespace::add(const Variable &v) {
	// TODO: Remove? This is done by the C++ compiler and Storm anyway.
	for (nat i = 0; i < owner.variables.size(); i++)
		if (owner.variables[i].name == v.name)
			throw Error(L"Member variable " + toS(v.name) + L" already declared!", v.pos);

	owner.variables.push_back(v);
}

void ClassNamespace::add(const Function &f) {
	if (f.name == Function::dtor)
		owner.dtorFound = true;

	Function g = f;
	g.name = owner.name + f.name;
	g.isMember = true;
	// Add our this-pointer.
	g.params.insert(g.params.begin(), new ResolvedType(&owner));
	world.functions.push_back(g);
}

/**
 * Primitive.
 */

Primitive::Primitive(const CppName &name, const String &pkg, const CppName &generate, const SrcPos &pos) :
	Type(name, pkg, pos), generate(generate) {}

void Primitive::print(wostream &to) const {
	to << L"primitive " << name;
}

void Primitive::resolveTypes(World &world) {
	map<String, Size>::const_iterator i = world.builtIn.find(name.last());
	if (i == world.builtIn.end())
		throw Error(L"Unknown built-in type: " + name, pos);

	mySize = i->second;
}

Size Primitive::size() const {
	return mySize;
}

bool Primitive::heapAlloc() const {
	return false;
}

void Primitive::ptrOffsets(vector<Offset> &append) const {
	// None.
}

void Primitive::scannedVars(vector<ScannedVar> &append) const {}

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

void Enum::scannedVars(vector<ScannedVar> &append) const {}

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
