#include "stdafx.h"
#include "Type.h"
#include "Exception.h"
#include "Engine.h"
#include "Lib/Object.h"
#include "Std.h"
#include "Package.h"
#include "Function.h"

namespace storm {

	const String Type::CTOR = L"__ctor";
	const String Type::DTOR = L"__dtor";

	Type::Type(const String &name, TypeFlags f)
		: Named(name), engine(Object::engine()), flags(f), typeRef(engine.arena, L"typeRef"),
		  mySize(), chain(this), vtable(engine) {

		init();
	}

	Type::Type(const String &name, TypeFlags f, Size size, void *cppVTable)
		: Named(name), engine(Object::engine()), flags(f), typeRef(engine.arena, L"typeRef"),
		  mySize(size), chain(this), vtable(engine) {

		// Create the VTable first, otherwise init() may get strange ideas...
		vtable.create(cppVTable);

		init();
	}

	void Type::init() {
		lazyLoading = false;
		lazyLoaded = false;
		parentPkg = null;

		typeRef.set(this);

		// Enforce that all objects inherit from Object.
		if (flags & typeClass) {
			// NOTE: The only time objType will be null is during startup, and in
			// that case, setSuper will be called later anyway. Therefore it is OK
			// to leave the class without a Object parent for a short while.
			Type *objType = Object::type(engine);
			if (objType)
				setSuper(objType);
		}
	}

	Type::~Type() {}

	void Type::allowLazyLoad(bool allow) {
		lazyLoading = !allow;
	}

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
			updateVirtual();
			if (Type *s = super())
				s->updateVirtual();
		}
		lazyLoaded = true;
	}

	void Type::clear() {
		members.clear();
		chain.super(null);
	}

	Type *Type::createType(Engine &engine, const String &name, TypeFlags flags) {
		void *mem = allocDumb(engine, sizeof(Type));
		size_t typeOffset = OFFSET_OF(Object, myType);
		size_t engineOffset = sizeof(Named);
		OFFSET_IN(mem, typeOffset, Type *) = (Type *)mem;
		OFFSET_IN(mem, engineOffset, Engine *) = &engine;
		return new (mem) Type(name, flags, Size(sizeof(Type)), Type::cppVTable());
	}

	bool Type::isA(Type *o) const {
		return chain.isA(o);
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

			Size s = superSize();
			mySize = layout.size(s);
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

		Type *lastSuper = chain.super();

		chain.super(super);

		if (lastSuper)
			lastSuper->updateVirtual();

		if (!vtable.builtIn()) {
			if (super) {
				vtable.create(super->vtable);
				super->updateVirtual();
			} else {
				vtable.create();
			}
		}
	}

	Type *Type::super() const {
		return chain.super();
	}

	Named *Type::find(const Name &name) {
		if (name.size() != 1)
			return null;

		if (!lazyLoading)
			ensureLoaded();

		MemberMap::const_iterator i = members.find(name[0]);
		if (i != members.end())
			return i->second.borrow();

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

		if (Type *s = super())
			s->updateVirtual();
		updateVirtual(ovl);
	}

	void Type::validate(NameOverload *o) {
		if (o->params.empty())
			throw TypedefError(L"Member functions must have at least one parameter!");

		const Value &first = o->params.front();
		if (!first.canStore(Value(this, true)))
			throw TypedefError(L"Member functions must have 'this' as first parameter."
							L" Got " + ::toS(first) + L", expected " +
							::toS(Value(this)) + L" for " + o->name);

		if (o->name == CTOR) {
			Function *fn = as<Function>(o);
			if (!fn)
				throw TypedefError(L"Constructors must be functions.");
			if (fn->result != Value())
				throw TypedefError(L"Constructors may not return a value.");
		}

		if (o->name == DTOR) {
			Function *fn = as<Function>(o);
			if (!fn)
				throw TypedefError(L"Destructors must be functions.");
			if (fn->result != Value())
				throw TypedefError(L"Destructors may not return a value.");
			if (fn->params.size() != 1 || fn->params[0].type != this)
				throw TypedefError(L"Destructors may take no parameters except the this-parameter.");
		}
	}

	Function *Type::destructor() {
		Overload *o = as<Overload>(find(Name(DTOR)));
		if (!o)
			return null;

		return as<Function>(o->find(vector<Value>(1, Value(this, true))));
	}

	Offset Type::offset(const TypeVar *var) const {
		return layout.offset(superSize(), var);
	}

	void Type::updateVirtual() {
		// Value types does not need vtables.
		if (flags & typeValue)
			return;

		for (MemberMap::iterator i = members.begin(), end = members.end(); i != end; ++i) {
			Overload *o = i->second.borrow();
			updateVirtual(o);
		}
	}

	void Type::updateVirtual(Overload *o) {
		// Constructors never need virtual dispatch (and would crash if we used it there...).
		if (o->name == CTOR)
			return;

		for (Overload::iterator i = o->begin(), end = o->end(); i != end; ++i) {
			NameOverload *no = i->borrow();
			if (Function *fn = as<Function>(no)) {
				if (needsVirtual(fn))
					enableLookup(fn);
				else
					disableLookup(fn);
			}
		}
	}

	void Type::enableLookup(Function *fn) {
		VTablePos pos = vtable.insert(fn);
		insertOverloads(fn);

		// PLN(*fn << " got " << pos);
		// vtable.dbg_dump();

		// If we're a built in type, we do not want to mess with the lookup. A great one is already provided
		// by the C++ compiler!
		if (!vtable.builtIn()) {
			Auto<DelegatedCode> lookup = CREATE(DelegatedCode, engine, engine.virtualCall(pos), identifier());
			fn->setLookup(lookup);
		}
	}

	void Type::disableLookup(Function *fn) {
		// TODO: Remove 'fn' from the vtable when we know it is not needed there anymore.

		// Do not mess with vtables in C++-derived classes. Should not cause troubles,
		// but we would lose the C++ vtable call stub and replace it with our own slower (and possibly wrong) version.
		if (!vtable.builtIn())
			fn->setLookup(null);
	}

	bool Type::needsVirtual(Function *fn) {
		vector<Type *> children = chain.children();
		for (nat i = 0; i < children.size(); i++) {
			Type *c = children[i];
			if (c->overloadTo(fn))
				return true;
			if (c->needsVirtual(fn))
				return true;
		}

		return false;
	}

	Function *Type::overloadTo(Function *to) {
		MemberMap::const_iterator i = members.find(to->name);
		if (i == members.end())
			return null;

		// Replace the 'this' parameter, otherwise we would never get a match!
		vector<Value> params = to->params;
		params[0].type = this;
		return as<Function>(i->second->find(params));
	}

	void Type::insertOverloads(Function *fn) {
		if (Function *f = overloadTo(fn))
			if (f != fn)
				vtable.insert(f);

		vector<Type *> children = chain.children();
		for (nat i = 0; i < children.size(); i++)
			children[i]->insertOverloads(fn);
	}

}
