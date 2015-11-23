#include "stdafx.h"
#include "Type.h"
#include "Exception.h"
#include "Engine.h"
#include "Lib/Object.h"
#include "Shared/TObject.h"
#include "Shared/CloneEnv.h"
#include "Std.h"
#include "Package.h"
#include "Function.h"
#include "CodeGen.h"

namespace storm {

	const String Type::CTOR = L"__ctor";
	const String Type::DTOR = L"__dtor";

	static TypeFlags maskFlags(TypeFlags flags) {
		// We do not want to store the typeManualSuper flag! That is only for the init function.
		return flags & ~typeManualSuper;
	}

	Type::Type(const String &name, TypeFlags f, const vector<Value> &params, Size size)
		: NameSet(name, params), engine(Object::engine()), typeFlags(maskFlags(f)), typeRef(engine.arena, L"typeRef:" + name),
		  mySize(size), chain(this), vtable(engine) {

		init(f);
	}

	Type::Type(const String &name, TypeFlags f, Size size, void *cppVTable)
		: NameSet(name), engine(Object::engine()), typeFlags(maskFlags(f)), typeRef(engine.arena, L"typeRef:" + name),
		  mySize(size), chain(this), vtable(engine) {

		// Create the VTable first, otherwise init() may get strange ideas...
		vtable.createCpp(cppVTable);

		init(f);
	}

	void Type::init(TypeFlags typeFlags) {
		hashAsRef = null;
		equalsAsRef = null;
		handleDefaultCtorFn = null;

		typeRef.setPtr(this);
		typeHandle = new RefHandle(*typeRef.contents());

		// Generally, we want this on types!
		flags |= namedMatchNoInheritance;

		if (typeFlags & typeRawPtr) {
			assert(typeFlags & typeClass, L"typeRawPtr has to be used with typeClass");
		}


		// Enforce that all objects inherit from Object. Note that the flag 'typeManualSuper' is
		// set when we create Types before the Object::type has been initialized.
		if ((typeFlags & typeManualSuper) == 0) {
			if (typeFlags & typeClass) {
				assert(Object::stormType(engine), L"Please use the 'typeManualSuper' flag before Object has been created.");
				setSuper(Object::stormType(engine));
			}
		}
	}

	Type::~Type() {
		delete hashAsRef;
		delete equalsAsRef;
		delete handleDefaultCtorFn;

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
		void *mem = noTypeAlloc(engine, sizeof(Type)).mem;
		size_t typeOffset = OFFSET_OF(Object, myType);
		size_t engineOffset = sizeof(NameSet);
		OFFSET_IN(mem, typeOffset, Type *) = (Type *)mem;
		OFFSET_IN(mem, engineOffset, Engine *) = &engine;
		// We do not need to use the unsafe version here, we set the type!
		return new (mem) Type(name, flags, Size(sizeof(Type)), Type::cppVTable());
	}

	bool Type::isA(const Type *o) const {
		return chain.isA(o);
	}

	int Type::distanceFrom(Type *o) const {
		return chain.distance(o);
	}

	Size Type::superSize() const {
		if (super())
			return super()->size();

		if (typeFlags & typeClass) {
			if (this == Object::stormType(engine))
				return objectBaseSize();
			else if (this == TObject::stormType(engine))
				return tObjectBaseSize();
		} else if (typeFlags & typeValue) {
			return Size(0);
		}

		assert(false);
		return Size();
	}

	Size Type::size() {
		if (mySize == Size()) {
			const os::Thread &t = TObject::thread->thread();
			if (t == os::Thread::current()) {
				// Already on the Compiler thread.
				forceLoad();

				Size s = superSize();
				mySize = layout.size(s);
			} else {
				// We need to run on the Compiler thread. Note the 'Semaphore', this is to ensure
				// that we do not break any semantics regarding to the threading model.
				os::Future<Size, Semaphore> f;
				os::FnParams p; p.add(this);
				os::UThread::spawn(address(&Type::size), true, p, f, &t);
				return f.result();
			}
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

		typeHandle->isFloat = builtInType() == BasicTypeInfo::floatNr;

		if (typeFlags & typeClass) {
			// Borrow the copy, destroy, and create from regular handles. They are doing the right
			// thing if we're an object!
			const Handle &t = storm::handle<Auto<Object> >();

			typeHandle->size = t.size;
			typeHandle->createRaw(t.create);
			typeHandle->destroyRaw(t.destroy);
			typeHandle->deepCopyRaw(t.deepCopy);
		} else {
			Function *dtor = destructor();
			Function *create = copyCtor();
			Function *deepCopy = deepCopyFn();
			if (!create)
				throw RuntimeError(L"The type " + identifier() + L" does not have a copy constructor.");

			typeHandle->size = size().current();
			typeHandle->createRef(create->ref());

			if (dtor)
				typeHandle->destroyRef(dtor->ref());
			else
				typeHandle->destroyRef();

			if (deepCopy)
				typeHandle->deepCopyRef(deepCopy->ref());
			else
				typeHandle->deepCopyRef();
		}

		Function *equals = equalsFn();
		Function *hash = hashFn();

		// TODO? Use the defaults when we're an object.
		if (equals) {
			if (!equalsAsRef)
				equalsAsRef = new AsRef(equals);
			typeHandle->equalsRef(equalsAsRef->source);
		} else {
			typeHandle->equalsRef();
		}

		if (hash) {
			if (!hashAsRef)
				hashAsRef = new AsRef(hash);
			typeHandle->hashRef(hashAsRef->source);
		} else {
			typeHandle->hashRef();
		}

		updateHandleOutput();
	}

	void Type::updateHandleOutput() {
		// 1: Try to find the add() function for this type in StrBuf...
		Type *s = StrBuf::stormType(engine);
		if (Auto<Function> f = steal(s->findCpp(L"add", valList(2, Value::thisPtr(s), Value(this)))).as<Function>()) {
			typeHandle->outputRef(f->ref(), RefHandle::outputAdd);
			return;
		}

		// BEWARE: this may change over time, which is not properly handled right now.
		if (Function *f = findToSFn()) {
			if (f->isMember()) {
				// Always by-ref if we've found a member function.
				typeHandle->outputRef(f->ref(), RefHandle::outputByRefMember);
			} else {
				// We can always find a member function if 'this' is an object.
				assert(flags | typeValue, L"Should be able to find 'toS' as a member for objects! " + name);

				const Value &v = f->params[0];
				if (v.ref) {
					typeHandle->outputRef(f->ref(), RefHandle::outputByRef);
				} else {
					typeHandle->outputRef(f->ref(), RefHandle::outputByVal);
				}
			}

			return;
		}

		typeHandle->outputRef();
	}

	void Type::setSuper(Par<Type> super) {
		TypeFlags superFlags;
		if (super)
			superFlags = super->typeFlags;
		else
			superFlags = typeFlags & ~typeFinal;

		if (superFlags & typeFinal)
			throw InternalError(super->identifier() +
								L" is final and can not be inherited from by " +
								identifier());

		if (typeFlags & typeClass) {
			if (super == null && this != Object::stormType(engine) && this != TObject::stormType(engine))
				throw InternalError(identifier() + L" must at least inherit from Object. Not:" + super->identifier());
			if (!(superFlags & typeClass))
				throw InternalError(identifier() + L" may only inherit from other objects. Not: " + super->identifier());
		} else if (typeFlags & typeValue) {
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
		if (!(typeFlags & typeClass))
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

	Named *Type::find(Par<NamePart> part) {
		if (Named *n = NameSet::find(part))
			return n;

		if (part->name != CTOR)
			if (Type *s = super())
				return s->find(part);

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

		if (typeFlags & typeRawPtr)
			to << L" (raw class ptr)";
		else if (typeFlags & typeClass)
			to << L" (class)";
		else if (typeFlags & typeValue)
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
			throw TypedefError(L"Member functions must have at least one parameter! " + o->name + L" in " + name);

		const Value &first = o->params.front();
		if (!first.canStore(Value(this, true)))
			throw TypedefError(L"Member functions must have 'this' as first parameter."
							L" Got " + ::toS(first) + L", expected " +
							::toS(Value(this)) + L" for " + o->name);

		if (o->name == CTOR) {
			if (typeFlags & typeClass) {
				if (first.ref)
					throw TypedefError(L"Class constructors should not be declared to take references as "
									L"their first parameter. At: " + ::toS(*o));
			} else if (typeFlags & typeValue) {
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
		return steal(findCpp(DTOR, vector<Value>(1, Value::thisPtr(this)))).as<Function>().borrow();
	}

	Function *Type::copyCtor() {
		return steal(findCpp(CTOR, vector<Value>(2, Value::thisPtr(this)))).as<Function>().borrow();
	}

	const void *Type::copyCtorFn() {
		// TODO: Cache this!
		return copyCtor()->pointer();
	}

	Function *Type::assignFn() {
		return steal(findCpp(L"=", vector<Value>(2, Value::thisPtr(this)))).as<Function>().borrow();
	}

	Function *Type::defaultCtor() {
		return steal(findCpp(CTOR, vector<Value>(1, Value::thisPtr(this)))).as<Function>().borrow();
	}

	Function *Type::deepCopyFn() {
		Auto<Named> n = findCpp(L"deepCopy", valList(2, Value::thisPtr(this), Value(CloneEnv::stormType(engine))));
		return n.as<Function>().borrow();
	}

	Function *Type::equalsFn() {
		// TODO: Clean up the equals-mess when we have figured out a suitable API!
		Auto<Named> n = findCpp(L"equals", valList(2, Value::thisPtr(this), Value::thisPtr(this)));
		Function *r = as<Function>(n.borrow());
		if (r)
			return r;
		n = findCpp(L"==", valList(2, Value::thisPtr(this), Value::thisPtr(this)));
		r = as<Function>(n.borrow());
		return r;
	}

	Function *Type::hashFn() {
		return steal(findCpp(L"hash", valList(1, Value::thisPtr(this)))).as<Function>().borrow();
	}

	code::RefSource *Type::handleDefaultCtor() {
		using namespace code;
		Function *wrap = defaultCtor();
		if (!wrap)
			return null;

		if (typeFlags & typeValue)
			return &wrap->ref();

		if (!handleDefaultCtorFn) {
			// Create it!
			Auto<CodeGen> g = CREATE(CodeGen, this, RunOn());
			Listing &l = g->l->v;

			Variable p = l.frame.createPtrParam();

			l << prolog();

			// Note: we're not using any automatic cleanup here. It will be immediatly moved to its
			// proper location with automatic cleanup already in place. Otherwise we would have to
			// call addRef one extra time.
			Variable v = l.frame.createPtrVar(l.frame.root());
			allocObject(g, wrap, vector<code::Value>(), v);
			l << mov(ptrA, p);
			l << mov(ptrRel(ptrA), v);

			l << epilog();
			l << ret(retVoid());

			handleDefaultCtorFn = new code::RefSource(engine.arena, identifier() + L"<defctor>");
			handleDefaultCtorFn->set(new Binary(engine.arena, l));
		}
		return handleDefaultCtorFn;
	}

	Function *Type::findToSFn() {
		// 1: Local scope.
		Auto<Named> n = findCpp(L"toS", valList(1, Value::thisPtr(this)));
		if (Function *f = as<Function>(n.borrow()))
			return f;

		// 2: Surrounding scope.
		n = parent()->findCpp(L"toS", valList(1, Value(this)));
		if (Function *f = as<Function>(n.borrow()))
			return f;

		// 3: Out of luck. We could look in language-specific parts here, but that gets really messy!
		return null;
	}


	Offset Type::offset(const TypeVar *var) const {
		return layout.offset(superSize(), var);
	}

	vector<Auto<TypeVar> > Type::variables() const {
		return layout.variables();
	}

	void Type::updateVirtual() {
		// Value types does not need vtables.
		if (typeFlags & typeValue)
			return;
		if (typeFlags & typeRawPtr)
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

		if (typeFlags & typeValue)
			return;
		if (typeFlags & typeRawPtr)
			return;

		Function *fn = as<Function>(named);
		if (fn == null)
			return;

		updateVirtualHere(fn);

		for (Type *s = super(); s; s = s->super()) {
			Auto<Function> base = s->overloadTo(fn);
			if (base) {
				if (base->flags & namedFinal)
					// TODO: Refer a location!
					throw TypedefError(L"The function " + named->identifier() + L" overloads the final function " +
									base->identifier() + L".");

				s->enableLookup(base.borrow());
			}
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

		// If we're a built in type, we do not want to mess with the lookup, since a great one is
		// already provided by the C++ compiler! Aside from that, it will destroy the special lookup
		// for the destructor.
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
			if (steal(c->overloadTo(fn)))
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
		Auto<NamePart> part = CREATE(NamePart, this, to->name, params);
		Auto<Function> match = steal(NameSet::tryFind(part)).as<Function>();
		if (to == match)
			return null;
		return match.ret();
	}

	void Type::insertOverloads(Function *fn) {
		if (Auto<Function> f = overloadTo(fn))
			if (f != fn)
				VTablePos pos = vtable.insert(f.borrow());

		vector<Type *> children = chain.children();
		for (nat i = 0; i < children.size(); i++)
			children[i]->insertOverloads(fn);
	}

	// This is used in the DLL interface. It is declared in Shared/Types.h
	const Handle &typeHandle(Type *t) {
		return t->handle();
	}

	Function *emptyCtor(Par<Type> t) {
		return capture(t->defaultCtor()).ret();
	}

	Function *dtor(Par<Type> t) {
		return capture(t->destructor()).ret();
	}

	Function *copyCtor(Par<Type> t) {
		return capture(t->copyCtor()).ret();
	}

	Function *assignFn(Par<Type> t) {
		return capture(t->assignFn()).ret();
	}

	Function *deepCopyFn(Par<Type> t) {
		return capture(t->deepCopyFn()).ret();
	}

	Function *STORM_FN equalsFn(Par<Type> t) {
		return capture(t->equalsFn()).ret();
	}

	Function *STORM_FN hashFn(Par<Type> t) {
		return capture(t->hashFn()).ret();
	}

	Function *STORM_FN findToSFn(Par<Type> t) {
		return capture(t->findToSFn()).ret();
	}

	wrap::Size STORM_FN size(Par<Type> t) {
		return t->size();
	}

}
