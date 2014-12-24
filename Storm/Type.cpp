#include "stdafx.h"
#include "Type.h"
#include "Exception.h"
#include "Engine.h"
#include "Lib/Object.h"
#include "Std.h"
#include "Package.h"

namespace storm {

	const String Type::CTOR = L"__ctor";

	Type::Type(const String &name, TypeFlags f, nat size)
		: Named(name), engine(Object::engine()), flags(f), fixedSize(size), mySize(0), parentPkg(null) {
		superTypes.push_back(this);
	}

	Type::~Type() {}

	void Type::clear() {
		members.clear();
		superTypes = vector<Type *>(1, this);
	}

	Type *Type::createType(Engine &engine, const String &name, TypeFlags flags) {
		void *mem = allocDumb(engine, sizeof(Type));
		size_t typeOffset = OFFSET_OF(Object, myType);
		size_t engineOffset = sizeof(Named);
		OFFSET_IN(mem, typeOffset, Type *) = (Type *)mem;
		OFFSET_IN(mem, engineOffset, Engine *) = &engine;
		return new (mem) Type(name, flags, sizeof(Type));
	}

	bool Type::isA(Type *o) const {
		nat depth = o->superTypes.size();
		return superTypes.size() >= depth
			&& superTypes[depth - 1] == o;
	}

	nat Type::superSize() const {
		if (super())
			return super()->size();

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
				TODO(L"Implement me! (for " << identifier() << ")");
			}
		}
		return mySize;
	}

	void Type::setSuper(Type *super) {
		assert(super->flags == flags);

		Type *oldSuper = this->super();
		if (oldSuper != null && oldSuper != super)
			oldSuper->children.erase(this);
		if (oldSuper != super && super != null)
			super->children.insert(this);

		if (super == null) {
			superTypes = vector<Type*>(1, this);
		} else {
			nat sz = super->superTypes.size();
			superTypes = vector<Type *>(sz + 1);
			for (nat i = 0; i < sz; i++) {
				superTypes[i] = super->superTypes[i];
			}
			superTypes[sz] = this;
		}

		// Notify our children we've our type map!
		for (set<Type*>::iterator i = children.begin(); i != children.end(); ++i) {
			(*i)->setSuper(this);
		}
	}

	Type *Type::super() const {
		if (superTypes.size() <= 1)
			return null;
		else
			return superTypes[superTypes.size() - 2];
	}

	Named *Type::find(const Name &name) {
		if (name.size() != 1)
			return null;

		MemberMap::const_iterator i = members.find(name[0]);
		if (i != members.end())
			return i ->second.borrow();

		if (Type *s = super())
			return s->find(name);

		return null;
	}

	void Type::output(wostream &to) const {
		to << name;
		if (super())
			to << L" : " << super()->path();
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
			ovl = CREATE(Overload, engine, this, o->name);
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
			if (!first.canStore(Value(this, true)))
				throw TypedefError(L"Member functions must have 'this' as first parameter."
								L" Got " + ::toS(first) + L", expected " +
								::toS(Value(this)) + L" for " + o->name);
		}
	}

	code::Value Type::destructor() const {
		TODO(L"Implement!");
		assert(false);
		return code::Value();
	}

}
