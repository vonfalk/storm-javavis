#include "stdafx.h"
#include "Type.h"
#include "Exception.h"

namespace storm {

	const String Type::CTOR = L"__ctor";

	Type::Type(const String &name, TypeFlags f) : Named(name), flags(f), superType(null) {}

	Type::~Type() {
		clearMap(members);
	}

	nat Type::size() const {
		TODO(L"Implement me!");
		return 0;
	}

	void Type::setParentScope(Scope *parent) {
		nameFallback = parent;
	}

	void Type::setSuper(Type *super) {
		assert(super->flags == flags);
		assert(superType == null);
		superType = super;
	}

	Named *Type::findHere(const Name &name) {
		if (name.size() != 1)
			return null;

		MemberMap::const_iterator i = members.find(name[0]);
		if (i == members.end())
			return null;
		else
			return i->second;
	}

	void Type::output(wostream &to) const {
		to << name;
		if (superType)
			to << L" : " << superType->name;
		to << ":" << endl;
		Indent i(to);

		for (MemberMap::const_iterator i = members.begin(); i != members.end(); ++i) {
			to << *i->second;
		}
	}

	void Type::add(NameOverload *o) {
		validate(o);

		Overload *ovl = null;
		MemberMap::iterator i = members.find(o->name);
		if (i == members.end()) {
			ovl = new Overload(o->name);
			members.insert(make_pair(o->name, ovl));
		} else {
			ovl = i->second;
		}

		ovl->add(o);
	}

	void Type::validate(NameOverload *o) {
		if (o->params.empty())
			throw TypedefError(L"Member functions must have at least one parameter!");

		const Value &first = o->params.front();
		if (o->name == CTOR) {
			TODO("Better validation here!");
			if (first.type == null || first.type->name != L"Type")
				throw TypedefError(L"Member constructors must have 'Type' as first parameter.");
		} else {
			if (first != Value(this))
				throw TypedefError(L"Member functions must have 'this' as first parameter.");
		}
	}

	bool Type::operator <(const Type &o) const {
		// Ugly, but works for now.
		return this < &o;
	}

}
