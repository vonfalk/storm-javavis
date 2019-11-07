#include "stdafx.h"
#include "Type.h"
#include "TypeRef.h"
#include "Utils/Indent.h"
#include "Exception.h"
#include "World.h"

Type::Type(const CppName &name, const String &pkg, const SrcPos &pos, const Auto<Doc> &doc) :
	id(0), name(name), pkg(pkg), pos(pos), doc(doc), external(false) {}

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

Size Type::size() const {
	// Align properly as Visual Studio does.
	Size s = rawSize();
	s += s.align();
	return s;
}

wostream &operator <<(wostream &to, const Type &type) {
	type.print(to);
	return to;
}

/**
 * Class.
 */

Class::Class(const CppName &name, const String &pkg, const SrcPos &pos, const Auto<Doc> &doc) :
	Type(name, pkg, pos, doc), valueType(false), abstractType(false), parent(L""),
	hiddenParent(false), dtorFound(false), parentType(null), threadType(null) {}

void Class::resolveTypes(World &in) {
	CppName ctx = name;

	if (!parent.empty() && !hiddenParent)
		parentType = in.types.find(parent, ctx, pos);

	if (!thread.empty())
		threadType = in.threads.find(thread, ctx, pos);

	for (nat i = 0; i < variables.size(); i++)
		variables[i].resolveTypes(in, ctx);
}

bool Class::isActor() const {
	if (threadType)
		return true;
	if (Class *p = as<Class>(parentType))
		return p->isActor();
	return false;
}

Size Class::rawSize() const {
	Size s;

	if (parentType) {
		// This part is interesting. Visual Studio acts as if the parent type is a data member of a
		// derived class, while GCC acts as if the data members of the parent were embedded directly
		// inside the derived class. This means that a derived class in GCC could be the same size
		// as the parent class even though it contains additional data members. (eg Parent: void *,
		// int. Derived: int). In Visual Studio, the derived class is always larger unless it is
		// empty. We need to detect this, as we assume the (visible) binary layout of objects are
		// the same on different platforms. For now, we only instruct the user to insert more
		// padding, as we have no better way of dealing with the situation at the moment.

		// We will examine the first variable (if present) to see if the alignment differs between
		// the two cases. If it is the same, we're fine anyway so we don't have to issue an
		// error. This means that we can be somewhat more accepting since this is not an issue in
		// most cases (many classes start with a pointer anyway), and solving the issue would waste
		// quite a bit of memory on 32-bit systems.


		Size raw = parentType->rawSize(); // Align as GCC would do.
		s = raw + raw.align(); // Align as Visual Studio would do.

		if (variables.size() >= 1) {
			Size t = variables[0].type->size();
			raw += t.align();
			s += t.align();

			if (raw.size32() != s.size32() || raw.size64() != s.size64()) {
				PVAR(*parentType);
				std::wostringstream msg;
				msg << L"The type " << name << L" will be inconsistently aligned on different systems. "
					<< L"On Windows, it will start at " << s << L" and on UNIX it will start at " << raw
					<< L". This is because the parent class, " << parentType->name << L" is aligned at "
					<< parentType->rawSize() << L". You can fix this by placing a pointer as the first "
					<< L"member of " << name << L", or by rearranging/padding " << parentType->name
					<< L" so that its size matches its natural alignment.";
				throw Error(msg.str(), pos);
			}
		} else {
			// Pass the raw size on so that we can detect any problems there!
			s = raw;
		}
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
	Size data = baseOffset();

	if (parentType) {
		parentType->ptrOffsets(to);
	}

	for (nat i = 0; i < variables.size(); i++) {
		const Auto<TypeRef> &t = variables[i].type;
		Offset offset = varOffset(i, data);

		if (isGcPtr(t)) {
			// TODO? Export all pointers, regardless if they are to a GC:d object or not?
			to.push_back(offset);
		} else if (Auto<ResolvedType> res = t.as<ResolvedType>()) {
			// Inline this type into ourselves.
			vector<Offset> o = res->type->ptrOffsets();
			for (nat i = 0; i < o.size(); i++)
				to.push_back(o[i] + offset);
		}
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

Size Class::baseOffset() const {
	if (parentType) {
		Size s = parentType->size();
		s += s.align();
		return s;
	} else if (!valueType) {
		return Size::sPtr; // vtable
	} else {
		return Size();
	}
}

Offset Class::varOffset(nat i, Size &base) const {
	const Auto<TypeRef> &t = variables[i].type;
	Size varSize = t->size();

	// Make sure to properly align the variable.
	base += varSize.align();
	Offset result(base);

	// Increase the size for next time.
	base += varSize;
	return result;
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

	if (f.isAbstract && !owner.abstractType)
		throw Error(L"The member function \"" + f.name + L"\" is marked abstract, "
					L"but the class is not marked with STORM_ABSTRACT_CLASS.", f.pos);

	Function g = f;
	g.name = owner.name + f.name;
	if (g.isStatic) {
		// Static functions are not treated as member functions. They just happen to be located
		// inside a class.
		g.pkg += L"." + owner.name.last();
	} else {
		g.isMember = true;
		// Add our this-pointer.
		g.params.insert(g.params.begin(), new RefType(new ResolvedType(&owner)));
		g.paramNames.insert(g.paramNames.begin(), L"this");
	}
	world.functions.push_back(g);
}

/**
 * Primitive.
 */

Primitive::Primitive(const CppName &name, const String &pkg, const CppName &generate, const SrcPos &pos, const Auto<Doc> &doc) :
	Type(name, pkg, pos, doc), generate(generate) {}

void Primitive::print(wostream &to) const {
	to << L"primitive " << name;
}

void Primitive::resolveTypes(World &world) {
	map<String, Size>::const_iterator i = world.builtIn.find(name.last());
	if (i == world.builtIn.end())
		throw Error(L"Unknown built-in type: " + name, pos);

	mySize = i->second;
}

Size Primitive::rawSize() const {
	return mySize;
}

bool Primitive::heapAlloc() const {
	return false;
}

void Primitive::ptrOffsets(vector<Offset> &append) const {
	// None.
}

void Primitive::scannedVars(vector<ScannedVar> &append) const {
	// None.
}

/**
 * Unknown primitive.
 */

UnknownPrimitive::UnknownPrimitive(const CppName &name, const String &pkg, const CppName &generate, const SrcPos &pos) :
	Type(name, pkg, pos, null), generate(generate) {}

void UnknownPrimitive::print(wostream &to) const {
	to << L"unknown primitive " << name;
}

void UnknownPrimitive::resolveTypes(World &world) {
	String last = name.last();
	for (nat i = 0; UnknownType::ids[i].name; i++) {
		if (last == UnknownType::ids[i].name) {
			mySize = UnknownType::ids[i].size;
			gcPtr = UnknownType::ids[i].gc;
			return;
		}
	}

	throw Error(L"Unknown built-in type: " + name, pos);
}

Size UnknownPrimitive::rawSize() const {
	return mySize;
}

bool UnknownPrimitive::heapAlloc() const {
	return false;
}

void UnknownPrimitive::ptrOffsets(vector<Offset> &append) const {
	if (gcPtr) {
		// This entire thing is a pointer!
		append.push_back(Offset());
	}
}

void UnknownPrimitive::scannedVars(vector<ScannedVar> &append) const {
	// None.
}

/**
 * Enum.
 */

Enum::Enum(const CppName &name, const String &pkg, const SrcPos &pos, const Auto<Doc> &doc) :
	Type(name, pkg, pos, doc), bitmask(false) {}

void Enum::resolveTypes(World &world) {}

Size Enum::rawSize() const {
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

Template::Template(const CppName &name, const String &pkg, const CppName &generator, const SrcPos &pos, const Auto<Doc> &doc) :
	name(name), pkg(pkg), generator(generator), pos(pos), doc(doc), external(false) {}
