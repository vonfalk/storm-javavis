#include "stdafx.h"
#include "Type.h"
#include "Exception.h"
#include "Engine.h"
#include "Lib/Object.h"
#include "Std.h"
#include "Package.h"

namespace storm {

	const String Type::CTOR = L"__ctor";

	Type::Type(const String &name, TypeFlags f, Size size)
		: Named(name), engine(Object::engine()), flags(f), fixedSize(size),
		  mySize(), parentPkg(null), lazyLoaded(false), lazyLoading(false) {
		if (flags & typeClass) {
			// NOTE: The only time objType will be null is during startup, and in
			// that case, setSuper will be called later anyway. Therefore it is OK
			// to leave the class without a Object parent for a short while.
			Type *objType = Object::type(engine);
			if (objType)
				superTypes.push_back(Object::type(engine));
		}
		superTypes.push_back(this);
	}

	Type::~Type() {}

	void Type::lazyLoad() {}

	void Type::ensureLoaded() {
		if (!lazyLoaded) {
			if (lazyLoading)
				throw InternalError(L"Cyclic loading of " + identifier() + L" detected!");

			lazyLoading = true;
			try {
				lazyLoad();
			} catch (...) {
				lazyLoading = false;
				throw;
			}
			lazyLoading = false;
		}
		lazyLoaded = true;
	}

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
		return new (mem) Type(name, flags, Size(sizeof(Type)));
	}

	bool Type::isA(Type *o) const {
		nat depth = o->superTypes.size();
		return superTypes.size() >= depth
			&& superTypes[depth - 1] == o;
	}

	Size Type::superSize() const {
		if (super())
			return super()->size();

		if (flags & typeClass)
			return Object::baseSize();
		else if (flags & typeValue)
			return Size(0);

		assert(false);
		return Size();
	}

	Size Type::size() {
		if (mySize == Size()) {
			ensureLoaded();

			if (fixedSize != Size()) {
				mySize = fixedSize;
			} else {
				Size s = superSize();
				mySize = layout.size(s);
			}
		}
		return mySize;
	}

	void Type::setSuper(Type *super) {
		if (super->flags & typeFinal)
			throw InternalError(super->identifier() +
							L" is final and can not be inherited from by " +
							identifier());

		if (flags & typeClass) {
			if (super == null && this != Object::type(engine))
				throw InternalError(identifier() + L" must at least inherit from Object.");
			if (!(super->flags & typeClass))
				throw InternalError(identifier() + L" may only inherit from other objects.");
		} else if (flags & typeValue) {
			if (!(super->flags & typeValue))
				throw InternalError(identifier() + L" may only inherit from other values.");
		}

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
		if (superTypes.size() <= 1) {
			return null;
		} else {
			return superTypes[superTypes.size() - 2];
		}
	}

	Named *Type::find(const Name &name) {
		if (name.size() != 1)
			return null;

		if (!lazyLoading)
			ensureLoaded();

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
		if (!lazyLoaded)
			to << L"(not loaded)";
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
		layout.add(o);
	}

	void Type::validate(NameOverload *o) {
		if (o->params.empty())
			throw TypedefError(L"Member functions must have at least one parameter!");

		const Value &first = o->params.front();
		if (!first.canStore(Value(this, true)))
			throw TypedefError(L"Member functions must have 'this' as first parameter."
							L" Got " + ::toS(first) + L", expected " +
							::toS(Value(this)) + L" for " + o->name);
	}

	code::Value Type::destructor() const {
		TODO(L"Implement!");
		// ensureLoaded(); Needed here!
		assert(false);
		return code::Value();
	}

}
