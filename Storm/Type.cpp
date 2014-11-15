#include "stdafx.h"
#include "Type.h"
#include "Exception.h"
#include "Engine.h"
#include "Lib/Object.h"
#include "Std.h"

namespace storm {

	const String Type::CTOR = L"__ctor";

	Type::Type(Engine &e, const String &name, TypeFlags f, nat size)
		: Named(name), engine(e), flags(f), superType(null), fixedSize(size), mySize(0) {}

	Type::~Type() {}

	void Type::clear() {
		members.clear();
		superType = null;
	}

	Type *Type::createType(Engine &engine, const String &name, TypeFlags flags) {
		void *mem = allocDumb(engine, sizeof(Type));
		size_t typeOffset = OFFSET_OF(Object, myType);
		OFFSET_IN(mem, typeOffset, Type *) = (Type *)mem;
		return new (mem) Type(engine, name, flags, sizeof(Type));
	}

	bool Type::isA(Type *o) const {
		for (const Type *curr = this; curr; curr = curr->super()) {
			if (o == curr)
				return true;
		}
		return false;
	}

	nat Type::superSize() const {
		if (superType)
			return superType->size();

		if (flags & typeClass)
			return sizeof(Object);
		else if (flags & typeValue)
			return 0;

		assert(false);
		return 0;
	}

	nat Type::size() const {
		if (mySize == 0) {
			if (fixedSize) {
				mySize = fixedSize;
			} else {
				nat s = superSize();
				// re-compute our size.
				// NOTE: base-classes must be able to propagate size-changes to children!
				TODO(L"Implement me!");
			}
		}
		return mySize;
	}

	void Type::setSuper(Type *super) {
		assert(super->flags == flags);
		assert(superType == null);
		superType = super;
	}

	Named *Type::find(const Name &name) {
		if (name.size() != 1)
			return null;

		MemberMap::const_iterator i = members.find(name[0]);
		if (i == members.end())
			return null;
		else
			return i->second.borrow();
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
			ovl = CREATE(Overload, engine, o->name);
			members.insert(make_pair(o->name, ovl));
		} else {
			ovl = i->second.borrow();
		}

		ovl->add(o);
	}

	void Type::validate(NameOverload *o) {
		if (o->params.empty())
			throw TypedefError(L"Member functions must have at least one parameter!");

		const Value &first = o->params.front();
		if (o->name == CTOR) {
			if (first.type != Type::type(engine))
				throw TypedefError(L"Member constructors must have 'Type' as first parameter.");
		} else {
			if (first != Value(this))
				throw TypedefError(L"Member functions must have 'this' as first parameter.");
		}
	}

}
