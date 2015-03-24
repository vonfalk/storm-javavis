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

	Type::Type(const String &name, TypeFlags f, const vector<Value> &params)
		: NameSet(name, params), engine(Object::engine()), flags(f), typeRef(engine.arena, L"typeRef"),
		  mySize(), chain(this), vtable(engine) {

		init();
	}

	Type::Type(const String &name, TypeFlags f, Size size, void *cppVTable)
		: NameSet(name), engine(Object::engine()), flags(f), typeRef(engine.arena, L"typeRef"),
		  mySize(size), chain(this), vtable(engine) {

		// Create the VTable first, otherwise init() may get strange ideas...
		vtable.create(cppVTable);

		init();
	}

	void Type::init() {
		lazyLoading = false;
		lazyLoaded = false;

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
		NameSet::clear();
		chain.super(null);
	}

	Type *Type::createType(Engine &engine, const String &name, TypeFlags flags) {
		void *mem = allocDumb(engine, sizeof(Type));
		size_t typeOffset = OFFSET_OF(Object, myType);
		size_t engineOffset = sizeof(NameSet);
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

	const Handle &Type::handle() {
		if (typeHandle.size == 0)
			updateHandle(true);
		return typeHandle;
	}

	void Type::updateHandle(bool force) {
		if (typeHandle.size == 0 && !force)
			return;

		Function *dtor = destructor();
		Function *create = copyCtor();
		if (!create)
			throw RuntimeError(L"The type " + identifier() + L" does not have a copy constructor.");

		typeHandle.size = size().current();
		if (dtor)
			typeHandle.destroyRef(dtor->ref());
		else
			typeHandle.destroyRef();
		typeHandle.createRef(create->ref());
	}

	void Type::setSuper(Par<Type> super) {
		TypeFlags superFlags;
		if (super)
			superFlags = super->flags;
		else
			superFlags = flags & ~typeFinal;

		if (superFlags & typeFinal)
			throw InternalError(super->identifier() +
								L" is final and can not be inherited from by " +
								identifier());

		if (flags & typeClass) {
			if (super == null && this != Object::type(engine))
				throw InternalError(identifier() + L" must at least inherit from Object. Not:" + super->identifier());
			if (!(superFlags & typeClass))
				throw InternalError(identifier() + L" may only inherit from other objects. Not: " + super->identifier());
		} else if (flags & typeValue) {
			if (!(superFlags & typeValue))
				throw InternalError(identifier() + L" may only inherit from other values. Not: " + super->identifier());
		}

		Type *lastSuper = chain.super();

		chain.super(super.borrow());

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

	void Type::setThread(Par<Thread> thread) {
		if (!(flags & typeClass))
			throw InternalError(identifier() + L": Can not be associated with a thread unless it is a class.");
		if (super() != Object::type(engine))
			throw InternalError(identifier() + L": Can not be associated with a thread since it is not a root object.");

		TODO(L"Implement me!");
	}

	Named *Type::findHere(const String &name, const vector<Value> &params) {
		if (!lazyLoading)
			ensureLoaded();

		if (Named *n = NameSet::findHere(name, params))
			return n;

		if (name != CTOR)
			if (Type *s = super())
				return s->findHere(name, params);

		return null;
	}

	void Type::output(wostream &to) const {
		to << name;
		if (super())
			to << L" : " << super()->identifier();
		if (!lazyLoaded)
			to << L"(not loaded)";
		to << ":";

		if (flags & typeClass)
			to << L" (class)";
		else if (flags & typeValue)
			to << L" (value)";

		to << endl;
		Indent i(to);

		NameSet::output(to);
	}

	void Type::add(Par<Named> o) {
		validate(o.borrow());

		NameSet::add(o);
		layout.add(o.borrow());

		// TODO: We do not need to update all virtual functions here.
		if (Type *s = super())
			s->updateVirtual();
		updateVirtual();
	}

	void Type::validate(Named *o) {
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
			if (fn->params.size() == 2 && fn->params[1].type == this && fn->params[1] != Value::thisPtr(this))
				throw TypedefError(L"The copy constructor must take a reference!");
		}

		if (o->name == DTOR) {
			Function *fn = as<Function>(o);
			if (!fn)
				throw TypedefError(L"Destructors must be functions.");
			if (fn->result != Value())
				throw TypedefError(L"Destructors may not return a value.");
			if (fn->params.size() != 1 || fn->params[0] != Value::thisPtr(this))
				throw TypedefError(L"Destructors may take no parameters except the this-parameter.");
		}
	}

	Function *Type::destructor() {
		return as<Function>(find(DTOR, vector<Value>(1, Value::thisPtr(this))));
	}

	Function *Type::copyCtor() {
		return as<Function>(find(CTOR, vector<Value>(2, Value::thisPtr(this))));
	}

	const void *Type::copyCtorFn() {
		return handle().create;
	}

	Function *Type::assignFn() {
		return as<Function>(find(L"=", vector<Value>(2, Value::thisPtr(this))));
	}

	Function *Type::defaultCtor() {
		return as<Function>(find(CTOR, vector<Value>(1, Value::thisPtr(this))));
	}


	Offset Type::offset(const TypeVar *var) const {
		return layout.offset(superSize(), var);
	}

	vector<Auto<TypeVar> > Type::variables() const {
		return layout.variables();
	}

	void Type::updateVirtual() {
		// Value types does not need vtables.
		if (flags & typeValue)
			return;

		for (NameSet::iterator i = begin(), end = this->end(); i != end; ++i) {
			updateVirtual(i->borrow());
		}
	}

	void Type::updateVirtual(Named *named) {
		// Constructors never need virtual dispatch (and would crash if we used it there...).
		if (named->name == CTOR)
			return;

		if (Function *fn = as<Function>(named)) {
			if (needsVirtual(fn))
				enableLookup(fn);
			else
				disableLookup(fn);
		}
	}

	void Type::enableLookup(Function *fn) {
		VTablePos pos = vtable.insert(fn);
		insertOverloads(fn);


		// PLN(*fn << " got " << pos);
		// vtable.dbg_dump();

		// If we're a built in type, we do not want to mess with the lookup. A great one is already provided
		// by the C++ compiler! Aside from that, it will destroy the special lookup for the destructor.
		if (!vtable.builtIn() && pos.type != VTablePos::tNone) {
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
		// Replace the 'this' parameter, otherwise we would never get a match!
		vector<Value> params = to->params;
		params[0] = Value::thisPtr(this);
		return as<Function>(NameSet::findHere(to->name, params));
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
