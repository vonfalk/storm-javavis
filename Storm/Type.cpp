#include "stdafx.h"
#include "Type.h"
#include "Exception.h"
#include "Engine.h"
#include "Lib/Object.h"
#include "Lib/TObject.h"
#include "Lib/CloneEnv.h"
#include "Std.h"
#include "Package.h"
#include "Function.h"

namespace storm {

	const String Type::CTOR = L"__ctor";
	const String Type::DTOR = L"__dtor";

	static TypeFlags maskFlags(TypeFlags flags) {
		return flags & ~typeManualSuper;
	}

	Type::Type(const String &name, TypeFlags f, const vector<Value> &params, Size size)
		: NameSet(name, params), engine(Object::engine()), flags(maskFlags(f)), typeRef(engine.arena, L"typeRef"),
		  mySize(size), chain(this), vtable(engine) {

		init(f);
	}

	Type::Type(const String &name, TypeFlags f, Size size, void *cppVTable)
		: NameSet(name), engine(Object::engine()), flags(maskFlags(f)), typeRef(engine.arena, L"typeRef"),
		  mySize(size), chain(this), vtable(engine) {

		// Create the VTable first, otherwise init() may get strange ideas...
		vtable.createCpp(cppVTable);

		init(f);
	}

	void Type::init(TypeFlags flags) {
		typeRef.setPtr(this);
		typeHandle = new RefHandle(*typeRef.contents());

		if (flags & typeRawPtr) {
			assert(flags & typeClass, L"typeRawPtr has to be used with typeClass");
		}


		// Enforce that all objects inherit from Object. Note that the flag 'typeManualSuper' is
		// set when we create Types before the Object::type has been initialized.
		if ((flags & typeManualSuper) == 0) {
			if (flags & typeClass) {
				assert(Object::stormType(engine), L"Please use the 'typeManualSuper' flag before Object has been created.");
				setSuper(Object::stormType(engine));
			}
		}
	}

	Type::~Type() {
		// Clear any references from our vtable first.
		vtable.clearRefs();
		// Clear any ctors before the vtable dies.
		NameSet::clear();
		delete typeHandle;
	}

	bool Type::loadAll() {
		// We need to update the VTables on successful load!
		updateVirtual();
		return true;
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

		if (flags & typeClass) {
			if (this == Object::stormType(engine))
				return Object::baseSize();
			else if (this == TObject::stormType(engine))
				return TObject::baseSize();
		} else if (flags & typeValue) {
			return Size(0);
		}

		assert(false);
		return Size();
	}

	Size Type::size() {
		if (mySize == Size()) {
			forceLoad();

			Size s = superSize();
			mySize = layout.size(s);
		}
		return mySize;
	}

	const Handle &Type::handle() {
		if (typeHandle->size == 0)
			updateHandle(true);
		return *typeHandle;
	}

	void Type::updateHandle(bool force) {
		if (typeHandle->size == 0 && !force)
			return;

		Function *dtor = destructor();
		Function *create = copyCtor();
		Function *deepCopy = deepCopyFn();
		if (!create)
			throw RuntimeError(L"The type " + identifier() + L" does not have a copy constructor.");

		typeHandle->size = size().current();
		if (dtor)
			typeHandle->destroyRef(dtor->ref());
		else
			typeHandle->destroyRef();
		if (deepCopy)
			typeHandle->deepCopyRef(deepCopy->ref());
		else
			typeHandle->deepCopyRef();
		typeHandle->createRef(create->ref());
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
			if (super == null && this != Object::stormType(engine) && this != TObject::stormType(engine))
				throw InternalError(identifier() + L" must at least inherit from Object. Not:" + super->identifier());
			if (!(superFlags & typeClass))
				throw InternalError(identifier() + L" may only inherit from other objects. Not: " + super->identifier());
		} else if (flags & typeValue) {
			if (!(superFlags & typeValue))
				throw InternalError(identifier() + L" may only inherit from other values. Not: " + super->identifier());
		}

		assert(this != TObject::stormType(engine) || super == null, L"TObject may not inherit from anything!");

		Type *lastSuper = chain.super();

		chain.super(super.borrow());

		if (lastSuper)
			lastSuper->updateVirtual();

		if (super) {
			vtable.setParent(super->vtable);
			updateVirtual();
		} else {
			assert(false);
			// vtable.create();
		}
	}

	Type *Type::super() const {
		return chain.super();
	}

	void Type::setThread(Par<NamedThread> thread) {
		if (!(flags & typeClass))
			throw InternalError(identifier() + L": Can not be associated with a thread unless it is a class.");
		Type *s = super();
		if (s != Object::stormType(engine) && s != TObject::stormType(engine))
			throw InternalError(identifier() + L": Can not be associated with a thread since it is not a root object.");

		if (s != TObject::stormType(engine))
			setSuper(TObject::stormType(engine));

		this->thread = thread;
	}

	RunOn Type::runOn() const {
		Type *tObj = TObject::stormType(engine);

		for (const Type *t = this; t; t = t->super()) {
			if (t == tObj)
				// Not decided which is the actual thread.
				return RunOn(RunOn::runtime);

			if (t->thread)
				return RunOn(t->thread);
		}

		// We do not inherit from TObject.
		return RunOn(RunOn::any);
	}

	Named *Type::findHere(const String &name, const vector<Value> &params) {
		if (Named *n = NameSet::findHere(name, params))
			return n;

		if (name != CTOR)
			if (Type *s = super())
				return s->findHere(name, params);

		return null;
	}

	void Type::output(wostream &to) const {
		to << name;
		if (params.size() > 0) {
			to << L"(";
			join(to, params, L", ");
			to << L")";
		}

		if (super())
			to << L" : " << super()->identifier();
		to << ":";

		if (flags & typeRawPtr)
			to << L" (raw class ptr)";
		else if (flags & typeClass)
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

		updateVirtual(o.borrow());
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
			if (flags & typeClass) {
				if (first.ref)
					throw TypedefError(L"Class constructors should not be declared to take references as "
									L"their first parameter. At: " + ::toS(*o));
			} else if (flags & typeValue) {
				if (!first.ref)
					throw TypedefError(L"Value constructors should be declared to take references as "
									L"their first parameter. At: " + ::toS(*o));
			}

			Function *fn = as<Function>(o);
			if (!fn)
				throw TypedefError(L"Constructors must be functions. Not: " + ::toS(*o));
			if (fn->result != Value())
				throw TypedefError(L"Constructors may not return a value. At: " + ::toS(*o));
			if (fn->params.size() == 2 && fn->params[1].type == this && fn->params[1] != Value::thisPtr(this))
				throw TypedefError(L"The copy constructor must take a reference! At: " + ::toS(*o));
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

	Function *Type::deepCopyFn() {
		return as<Function>(find(L"deepCopy", valList(2, Value::thisPtr(this), Value(CloneEnv::stormType(engine)))));
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
		if (flags & typeRawPtr)
			return;

		for (NameSet::iterator i = begin(), end = this->end(); i != end; ++i) {
			updateVirtual(i->borrow());
		}

		// Update parents as well...
		if (Type *s = super())
			s->updateVirtual();
	}

	void Type::updateVirtual(Named *named) {
		// Constructors never need virtual dispatch (and would crash if we used it there...).
		if (named->name == CTOR)
			return;

		if (flags & typeValue)
			return;
		if (flags & typeRawPtr)
			return;

		Function *fn = as<Function>(named);
		if (fn == null)
			return;

		updateVirtualHere(fn);

		for (Type *s = super(); s; s = s->super()) {
			Function *base = s->overloadTo(fn);
			if (base)
				s->enableLookup(base);
		}
	}

	void Type::updateVirtualHere(Function *fn) {
		if (needsVirtual(fn))
			enableLookup(fn);
		else
			disableLookup(fn);
	}

	void Type::enableLookup(Function *fn) {
		VTablePos pos = vtable.insert(fn);
		insertOverloads(fn);

		// PLN(*fn << " got " << pos);
		// vtable.dbg_dump();

		// If we're a built in type, we do not want to mess with the lookup. A great one is already provided
		// by the C++ compiler! Aside from that, it will destroy the special lookup for the destructor.
		if (!vtable.builtIn() && pos.type != VTablePos::tNone) {
			Auto<DelegatedCode> lookup = CREATE(DelegatedCode, engine, engine.virtualCall(pos));
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
		Function *match = as<Function>(NameSet::tryFind(to->name, params));
		if (to == match)
			return null;
		return match;
	}

	void Type::insertOverloads(Function *fn) {
		if (Function *f = overloadTo(fn))
			if (f != fn)
				VTablePos pos = vtable.insert(f);

		vector<Type *> children = chain.children();
		for (nat i = 0; i < children.size(); i++)
			children[i]->insertOverloads(fn);
	}

}
