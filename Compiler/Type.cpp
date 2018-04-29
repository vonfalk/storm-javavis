#include "stdafx.h"
#include "Type.h"
#include "Engine.h"
#include "NamedThread.h"
#include "Function.h"
#include "Adapter.h"
#include "Exception.h"
#include "Package.h"
#include "Core/Str.h"
#include "Core/Handle.h"
#include "Core/Gen/CppTypes.h"
#include "Core/StrBuf.h"
#include "OS/UThread.h"
#include "OS/Future.h"
#include "Utils/Memory.h"

namespace storm {

	const wchar *Type::CTOR = S("__init");
	const wchar *Type::DTOR = S("__destroy");


	// Set 'type->type' to 'me' while forwarding 'name'. This has to be done before invoking the
	// parent constructor, since that relies on 'engine()' working properly, which is not the case
	// for the first object otherwise.
	static Str *setMyType(Str *name, Type *me, GcType *type, Engine &e) {
		// Set the type properly.
		type->type = me;

		// We need to set the engine as well. Should be the first member of this class.
		OFFSET_IN(me, sizeof(NameSet), Engine *) = &e;

		// Check to see if we succeeded!
		assert(&me->engine == &e, L"Type::engine must be the first data member declared in Type.");

		return name;
	}

	Type::Type(Str *name, TypeFlags flags) :
		NameSet(name),
		engine(RootObject::engine()),
		myGcType(null),
		tHandle(null),
		typeFlags(flags & ~typeCpp) {

		init(null);
	}

	Type::Type(Str *name, Array<Value> *params, TypeFlags flags) :
		NameSet(name, params),
		engine(RootObject::engine()),
		myGcType(null),
		tHandle(null),
		typeFlags(flags & ~typeCpp) {

		init(null);
	}

	Type::Type(Str *name, TypeFlags flags, Size size, GcType *gcType, const void *vtable) :
		NameSet(name),
		engine(RootObject::engine()),
		myGcType(gcType),
		tHandle(null),
		typeFlags(flags | typeCpp),
		mySize(size) {

		myGcType->type = this;
		init(vtable);
	}

	Type::Type(Str *name, Array<Value> *params, TypeFlags flags, Size size, GcType *gcType, const void *vtable) :
		NameSet(name, params),
		engine(RootObject::engine()),
		myGcType(gcType),
		tHandle(null),
		typeFlags(flags | typeCpp),
		mySize(size) {

		myGcType->type = this;
		init(vtable);
	}

	// We need to set myGcType->type first, therefore we call setMyType!
	Type::Type(Engine &e, TypeFlags flags, Size size, GcType *gcType) :
		NameSet(setMyType(null, this, gcType, e)), engine(e), myGcType(gcType),
		tHandle(null), typeFlags(typeClass | typeCpp) {

		init(null);
	}

	Type::~Type() {
		GcType *g = myGcType;
		myGcType = null;
		// Barrier here?

		// The GC will ignore these during shutdown, when it is crucial not to destroy anything too
		// early.
		engine.gc.freeType(g);
	}

	void Type::init(const void *vtable) {
		assert((typeFlags & typeValue) == typeValue || (typeFlags & typeClass) == typeClass, L"Invalid type flags!");
		if (rawPtr()) {
			assert(typeFlags & typeClass, L"typeRawPtr has to be used with typeClass");
		}

		// Generally, we want this on types.
		flags |= namedMatchNoInheritance;

		if (myGcType) {
			if (value())
				myGcType->kind = GcType::tArray;
			rawDtor = (DtorFn)myGcType->finalizer;
		}

		if (engine.has(bootTypes))
			vtableInit(vtable);

		if (engine.has(bootTemplates))
			lateInit();
	}

	void Type::vtableInit(const void *vtab) {
		if (value() || rawPtr())
			return;

		myVTable = new (engine) VTable(this);
		if (typeFlags & typeCpp) {
			myVTable->createCpp(vtab);
		} else if (Type *t = super()) {
			myVTable->createStorm(t->myVTable);
		} else {
			myVTable->createStorm(Object::stormType(engine)->myVTable);
		}
	}

	void Type::lateInit() {
		NameSet::lateInit();

		if (!chain) {
			chain = new (this) TypeChain(this);
			Type *def = defaultSuper();
			if (def)
				setSuper(def);
		}

		chain->lateInit();

		if (myVTable)
			myVTable->lateInit();
	}

	// Our finalizer.
	static void destroyType(Type *t) {
		t->~Type();
	}

	GcType *Type::makeType(Engine &e, const GcType *src) {
		GcType *r = e.gc.allocType(GcType::tType, src->type, src->stride, src->count + 1);

		r->finalizer = src->finalizer;
		r->offset[0] = OFFSET_OF(Type, myGcType);
		for (nat i = 0; i < src->count; i++)
			r->offset[i+1] = src->offset[i];

		return r;
	}

	Type *Type::createType(Engine &e, const CppType *type) {
		assert(wcscmp(type->name, S("Type")) == 0, L"storm::Type was not found!");
		assert(Size(type->size).current() == sizeof(Type),
			L"The computed size of storm::Type is wrong: " + ::toS(Size(type->size).current()) + L" vs " + ::toS(sizeof(Type)));

		// Generate our layout description for the GC:
		nat entries = 0;
		for (; type->ptrOffsets[entries] != CppOffset::invalid; entries++)
			;

		GcType *t = e.gc.allocType(GcType::tType, null, sizeof(Type), entries + 1);

		// Insert 'myGcType' as the first (special) pointer:
		t->offset[0] = OFFSET_OF(Type, myGcType);
		for (nat i = 0; i < entries; i++)
			t->offset[i+1] = Offset(type->ptrOffsets[i]).current();

		// Ensure we're finalized.
		t->finalizer = address(&destroyType);

		// Now we can allocate the type and let the constructor handle the rest!
		return new (e, t) Type(e, typeClass, Size(type->size), t);
	}

	void Type::setType(Object *onto) const {
		engine.gc.switchType(onto, myGcType);
	}

	Type *Type::defaultSuper() const {
		if (value() || rawPtr())
			return null;
		else if (useThread)
			return TObject::stormType(engine);
		else
			return Object::stormType(engine);
	}

	void Type::setSuper(Type *to) {
		// If 'to' is null: figure out what to inherit from.
		if (!to)
			to = defaultSuper();

		// So that Object does not inherit from TObject.
		if (to == this)
			return;

		if (!chain)
			chain = new (this) TypeChain(this);

		// Nothing to do?
		if (to == chain->super())
			return;

		if (to && !to->chain)
			to->chain = new (this) TypeChain(to);

		// Set the type-chain properly.
		Type *prev = super();
		chain->super(to);

		// Notify our old parent of removal of vtable members. We only need to do this for the
		// topmost class we're moving, as all other vtables will be deleted.
		vtableDetachedSuper(prev);

		// Propagate chainges to all children.
		updateSuper();
	}

	void Type::updateSuper() {
		Type *to = super();

		// Which thread to use?
		Type *tObj = TObject::stormType(engine);
		if (to != null && to->chain != null && tObj->chain != null) {
			if (!to->chain->isA(tObj)) {
				useThread = null;
			} else if (to != tObj) {
				useThread = to->useThread;
			}
			// Propagate to our children.
			notifyThread(useThread);
		}

		// Invalidate the GcType for this type.
		if (typeFlags & typeCpp) {
			// Type from C++. Nothing to do.
		} else if (myGcType != null) {
			// We're not a type from C++.
			GcType *old = myGcType;
			myGcType = null;
			engine.gc.freeType(old);
		}

		// TODO: Invalidate the Layout as well.
		// TODO: Invalidate the handle as well.

		if ((typeFlags & typeCpp) != typeCpp && !value() && !rawPtr()) {
			// Re-initialize the vtable.
			myVTable->createStorm(to->myVTable);
		}

		// Register all functions here and in our children as vtable calls.
		vtableNewSuper();

		// Recurse into children.
		TypeChain::Iter i = chain->children();
		while (Type *c = i.next())
			c->updateSuper();
	}

	void Type::setThread(NamedThread *thread) {
		useThread = thread;

		Type *def = defaultSuper();
		// Note: this function is used early enough so that 'chain' may not be initialized.
		if (!chain || !isA(def))
			setSuper(def);

		// Propagate the current thread to any child types.
		notifyThread(thread);
	}

	RunOn Type::runOn() {
		if (isA(TObject::stormType(engine))) {
			if (useThread)
				return RunOn(useThread);
			else
				return RunOn(RunOn::runtime);
		} else {
			return RunOn();
		}
	}

	Bool Type::abstract() {
		// Note: We always insert abstract functions in a vtable, even if it is not necessary at the
		// moment. This means that we can simply examine all slots in a vtable to determine if we're
		// abstract or not.
		VTable *vt = vtable();

		// If we don't have a VTable, then the concept of 'abstract' is not meaningful.
		if (!vt)
			return false;

		if (isAbstract != abstractUnknown)
			return isAbstract == abstractYes;

		// If any of the topmost functions are abstract, the entire class is abstract.
		Array<Function *> *fns = vt->allSlots();
		isAbstract = abstractYes;
		for (Nat i = 0; i < fns->count(); i++)
			if (fns->at(i)->flags & namedAbstract)
				return true;

		isAbstract = abstractNo;
		return false;
	}

	void Type::ensureNonAbstract(SrcPos pos) {
		if (!abstract())
			return;

		// Generate a nice error message...
		std::wostringstream msg;
		msg << L"The class " << identifier() << L" is abstract due to the following members:\n";

		VTable *vt = vtable();
		if (vt) {
			Array<Function *> *fns = vt->allSlots();
			for (Nat i = 0; i < fns->count(); i++)
				if (fns->at(i)->flags & namedAbstract)
					msg << L" " << fns->at(i)->shortIdentifier() << L"\n";
		}

		throw InstantiationError(pos, msg.str());
	}

	void Type::add(Named *item) {
		NameSet::add(item);

		if (Function *f = as<Function>(item)) {
			if (*f->name == CTOR)
				updateCtor(f);
			else if (*f->name != DTOR)
				vtableFnAdded(f);

			if (*f->name == DTOR)
				updateDtor(f);

			if (tHandle)
				updateHandle(f);
		}

		if ((typeFlags & typeCpp) != typeCpp) {
			// Tell the layout we found a new variable!
			if (!layout)
				layout = new (engine) Layout();
			layout->add(item);
		}
	}

	Named *Type::find(SimplePart *part, Scope source) {
		if (Named *n = NameSet::find(part, source))
			return n;

		// Constructors are not inherited.
		if (*part->name == CTOR)
			return null;

		if (Type *s = super())
			return s->find(part, source);
		else
			return null;
	}

	void Type::notifyAdded(NameSet *, Named *what) {
		if (Function *f = as<Function>(what)) {
			updateHandleToS(false, f);
		}
	}

	void Type::notifyThread(NamedThread *thread) {
		useThread = thread;

		if (chain != null) {
			TypeChain::Iter i = chain->children();
			while (Type *c = i.next()) {
				c->notifyThread(thread);
			}
		}
	}

	code::Ref Type::typeRef() {
		if (!selfRef) {
			StrBuf *name = new (this) StrBuf();
			*name << identifier() << S("<type>");
			selfRef = new (engine) code::RefSource(name->toS());
			selfRef->setPtr(this);
		}
		return code::Ref(selfRef);
	}

	code::TypeDesc *Type::typeDesc() {
		if (myTypeDesc) {
			// Just return the desc we have. It is an actor anyway.
			return myTypeDesc;
		}

		// We need to create the description. Switch threads if neccessary.
		const os::Thread &t = TObject::thread->thread();
		if (t != os::Thread::current()) {
			// Switch threads...
			Type *me = this;
			os::Future<code::TypeDesc *, Semaphore> f;
			os::FnCall<code::TypeDesc *, 1> p = os::fnCall().add(me);
			os::UThread::spawn(address(&Type::typeDesc), true, p, f, &t);
			return f.result();
		}

		// Double-check so that the desc is not created.
		if (!myTypeDesc)
			myTypeDesc = createTypeDesc();
		return myTypeDesc;
	}

	code::TypeDesc *Type::createTypeDesc() {
		if (!value()) {
			// If we're not a value, then we're a plain pointer.
			return engine.ptrDesc();
		}

		// We need to generate a TypeDesc for this value.
		// First: See if we're a complex type.
		Function *ctor = copyCtor();
		Function *dtor = destructor();

		Bool simple = true;
		if (ctor && !ctor->pure())
			simple = false;
		if (dtor && !dtor->pure())
			simple = false;

		// Complex types are actually fairly simple to handle.
		if (!simple) {
			if (ctor == null)
				throw TypedefError(L"The type " + ::toS(identifier()) +
								L" has a nontrivial destructor, but no constructor.");

			code::Ref d = dtor ? dtor->ref() : engine.ref(Engine::rFnNull);
			return new (this) code::ComplexDesc(size(), ctor->ref(), d);
		}

		// Generate a proper description of this type!
		return createSimpleDesc();
	}

	code::SimpleDesc *Type::createSimpleDesc() {
		forceLoad();
		Size mySize = size(); // Performs a layout of all variables if neccessary.
		Nat elems = populateSimpleDesc(null);

		if (elems == 0 && mySize != Size())
			throw TypedefError(::toS(identifier()) + L": Trying to generate a type description for an empty object "
							L"with a nonzero size. This is most likely not what you want. There are two possible "
							L"reasons for why this might happen: Either, you try to access the type description of "
							L"the type too early, or you are attempting to construct a non-standard type, "
							L"in which case you should override 'createSimpleDesc' as well.");

		code::SimpleDesc *desc = new (this) code::SimpleDesc(mySize, elems);
		populateSimpleDesc(desc);
		return desc;
	}

	static void merge(Offset offset, Nat &pos, MAYBE(code::SimpleDesc *) into, code::SimpleDesc *from) {
		if (!into) {
			pos += from->count();
			return;
		}

		for (Nat i = 0; i < from->count(); i++) {
			code::Primitive src = from->at(i);
			into->at(pos++) = src.move(src.offset() + offset);
		}
	}

	Nat Type::populateSimpleDesc(MAYBE(code::SimpleDesc *) into) {
		using namespace code;

		Nat pos = 0;

		if (Type *parent = super()) {
			TypeDesc *desc = parent->typeDesc();
			if (SimpleDesc *s = as<SimpleDesc>(desc))
				merge(Offset(), pos, into, s);
			else
				throw TypedefError(L"Can not produce a SimpleDesc when the parent type is not a simple type!");
		}

		Array<MemberVar *> *vars = variables();
		for (Nat i = 0; i < vars->count(); i++) {
			MemberVar *v = vars->at(i);
			Offset offset = v->offset();
			Value type = v->type;

			if (type.isHeapObj() || type.ref) {
				// This is a pointer to something.
				if (into)
					into->at(pos) = Primitive(primitive::pointer, Size::sPtr, offset);
				pos++;
				continue;
			}

			TypeDesc *original = type.type->typeDesc();
			if (PrimitiveDesc *p = as<PrimitiveDesc>(original)) {
				if (into)
					into->at(pos) = p->v.move(offset);
				pos++;
			} else if (SimpleDesc *s = as<SimpleDesc>(original)) {
				merge(offset, pos, into, s);
			} else if (ComplexDesc *c = as<ComplexDesc>(original)) {
				UNUSED(c);
				throw TypedefError(L"Can not produce a SimpleDesc from a type containing a complex type!");
			} else {
				throw TypedefError(L"Unknown type description: " + ::toS(original));
			}
		}

		if (into) {
			assert(pos == into->count(), L"A too small SimpleDesc provided.");
		}

		return pos;
	}

	void Type::doLayout() {
		// Make sure we're fully loaded.
		forceLoad();

		// No layout -> nothing to do.
		if (!layout)
			return;

		// We might as well compute our size while we're at it!
		mySize = layout->doLayout(superSize());
	}

	Array<MemberVar *> *Type::variables() const {
		if (layout) {
			return layout->variables();
		} else {
			// Fall back to examining the contents of the NameSet.
			Array<MemberVar *> *result = new (this) Array<MemberVar *>();
			for (Iter i = begin(); i != end(); ++i) {
				if (MemberVar *v = as<MemberVar>(i.v()))
					result->push(v);
			}
			return result;
		}
	}

	Size Type::superSize() {
		if (super())
			return super()->size();

		if (value() || rawPtr())
			return Size();

		assert(false, L"We are a class which does not inherit from TObject or Object!");
		return Size();
	}

	Size Type::size() {
		if (mySize != Size())
			return mySize;

		// We need to compute our size. Switch threads if neccessary...
		const os::Thread &t = TObject::thread->thread();
		if (t != os::Thread::current()) {
			// Switch threads...
			Type *me = this;
			os::Future<Size, Semaphore> f;
			os::FnCall<Size, 1> p = os::fnCall().add(me);
			os::UThread::spawn(address(&Type::size), true, p, f, &t);
			return f.result();
		}

		// Recompute!
		forceLoad();
		mySize = superSize();
		if (layout) {
			// If we do not have a layout, we do not have any variables.
			mySize = layout->doLayout(mySize);
		}
		return mySize;
	}

	VTable *Type::vtable() {
		// Don't bother if we're a value or we don't have a vtable yet.
		if (!myVTable || value())
			return null;

		// If we're loaded, then we're good to go!
		if (allLoaded())
			return myVTable;

		// We need to load ourself. Make sure we're running on the proper thread.
		const os::Thread &t = TObject::thread->thread();
		if (t != os::Thread::current()) {
			// Switch threads...
			Type *me = this;
			os::Future<VTable *, Semaphore> f;
			os::FnCall<VTable *, 1> p = os::fnCall().add(me);
			os::UThread::spawn(address(&Type::vtable), true, p, f, &t);
			return f.result();
		}

		// In the proper thread. The only thing we need to do is to make sure that the VTable is
		// populated, which is simply done by issuing a 'forceLoad'. Us intercepting the 'add'
		// function will do all of the dirty work.
		// If we force loading too early, we will not be able to boot... All types used during
		// boot should be properly loaded during boot anyway, so that is fine.
		if (engine.has(bootDone))
			forceLoad();
		return myVTable;
	}

	static void defToS(const void *obj, StrBuf *to) {
		*to << S("<operator << not found>");
	}

	const Handle &Type::handle() {
		// If we're completely done with the handle, return it immediatly.
		// If it is 'toSMissing', there is nothing we can do but wait until we get notified by the parent.
		if (tHandle && (handleToS == toSFound || handleToS == toSMissing))
			return *tHandle;

		// We need to create the handle. Switch threads.
		const os::Thread &t = TObject::thread->thread();
		if (t != os::Thread::current()) {
			// Switch threads...
			Type *me = this;
			os::Future<const Handle *, Semaphore> f;
			os::FnCall<const Handle *, 1> p = os::fnCall().add(me);
			os::UThread::spawn(address(&Type::handle), true, p, f, &t);
			return *f.result();
		}

		if (!tHandle)
			buildHandle();
		if (handleToS != toSFound)
			// NOTE: This does not really help when 'handleToS == toSMissing'...
			updateHandleToS(false, null);
		return *tHandle;
	}

	static void CODECALL stormDtor(RootObject *object) {
		Type *t = runtime::typeOf(object);
		if (!t)
			return;

		Type::DtorFn f = t->rawDestructor();
		if (!f)
			return;

		// If shutting down, do not care about threads anymore, as we might not have enough to see
		// if the type is a TObject or not...
		if (!t->engine.has(bootShutdown)) {
			if (t->isA(TObject::stormType(t->engine))) {
				// We might need to switch threads...
				TObject *obj = (TObject *)object;
				if (obj->thread) {
					os::Thread t = obj->thread->thread();
					if (t != os::Thread::current()) {
						// Post a message to that thread (no need to wait).
						os::FnCall<void, 1> params = os::fnCall().add(object);
						os::UThread::spawn((const void *)f, true, params, &t);
						return;
					}
				}
			}
		}

		// If we get here, we shall execute on the current thread.
		(*f)(object);
	}

	void Type::updateDtor(Function *dtor) {
		// Nothing interesting happens until here.
		if (!engine.has(bootDone))
			return;

		if (dtor)
			rawDtor = (DtorFn)dtor->directRef().address();
		else
			rawDtor = null;

		if (!value() && myGcType)
			myGcType->finalizer = address(&stormDtor);
	}

	void Type::updateCtor(Function *ctor) {
		// Clear the cache.
		rawCtor = null;
	}

	Type::CopyCtorFn Type::rawCopyConstructor() {
		CopyCtorFn r = rawCtor;
		if (r)
			return r;

		// Call on proper thread...
		const os::Thread &t = TObject::thread->thread();
		if (t != os::Thread::current()) {
			// Wrong thread, switch!
			Type *me = this;
			os::Future<void *, Semaphore> f;
			os::FnCall<void *, 1> p = os::fnCall().add(me);
			os::UThread::spawn(address(&Type::rawCopyConstructor), true, p, f, &t);
			return (CopyCtorFn)f.result();
		}

		// Find the copy constructor...
		Function *ctor = copyCtor();
		if (!ctor)
			return null;

		rawCtor = (CopyCtorFn)ctor->ref().address();
		return rawCtor;
	}

	void Type::useSuperGcType() {
		if (myGcType)
			return;

		Type *s = super();
		assert(s, L"Can not use 'useSuperGcType' without a super class!");

		// DebugBreak();
		mySize = s->size();
		myGcType = engine.gc.allocType(s->gcType());
		myGcType->type = this;
	}

	const GcType *Type::gcType() {
		// Already created?
		if (myGcType != null)
			return myGcType;

		// We need to create it. Switch threads if neccessary...
		const os::Thread &t = TObject::thread->thread();
		if (t != os::Thread::current()) {
			// Wrong thread, switch!
			Type *me = this;
			os::Future<const GcType *, Semaphore> f;
			os::FnCall<const GcType *, 2> p = os::fnCall().add(me);
			os::UThread::spawn(address(&Type::gcType), true, p, f, &t);
			return f.result();
		}

		// We're on the correct thread. Compute the type!
		assert((typeFlags & typeCpp) != typeCpp, L"C++ types should be given a GcType on creation!");

		// Make sure everything is loaded.
		forceLoad();

		// Compute the GcType for us!
		const GcType *superGc = null;
		Size superSize;
		if (Type *s = super()) {
			superGc = s->gcType();
			superSize = s->size();
		}

		if (!layout) {
			// We do not have any variables of our own.
			if (superGc)
				myGcType = engine.gc.allocType(superGc);
			else if (value())
				myGcType = engine.gc.allocType(GcType::tArray, this, size().current(), 0);
			else if (rawPtr())
				myGcType = engine.gc.allocType(&pointerArrayType);
			else
				assert(false, L"We're a non-value not inheriting from Object or TObject!");

		} else {
			// Merge our parent's and our offsets (if we have a parent).
			nat entries = layout->fillGcType(superSize, superGc, null);

			if (value())
				myGcType = engine.gc.allocType(GcType::tArray, this, size().current(), entries);
			else if (superGc)
				myGcType = engine.gc.allocType(GcType::Kind(superGc->kind), this, size().current(), entries);
			else
				assert(false, L"Neither a value nor have a parent!");

			layout->fillGcType(superSize, superGc, myGcType);
		}

		myGcType->type = this;

		// Do we need a destructor?
		if (Function *dtor = destructor())
			updateDtor(dtor);

		return myGcType;
	}

	const GcType *Type::gcArrayType() {
		if (value()) {
			return gcType();
		} else if (useThread) {
			return engine.tObjHandle().gcArrayType;
		} else {
			return &pointerArrayType;
		}
	}

	static void objDeepCopy(void *obj, CloneEnv *env) {
		Object *&o = *(Object **)obj;
		cloned(o, env);
	}

	static void objToS(const void *obj, StrBuf *to) {
		const Object *o = *(const Object **)obj;
		*to << o;
	}

	void Type::buildHandle() {
		bool val = value() || rawPtr();

		if (!val) {
			// The above check is not strictly necessary for correctness. However, we crash during
			// boot if we try to access 'chain' too early.
			if (isA(TObject::stormType(engine))) {
				// TObject.
				tHandle = &engine.tObjHandle();
				return;
			}
		}

		if (!handleContent)
			handleContent = new (engine) code::Content();

		RefHandle *h = new (engine) RefHandle(handleContent);
		tHandle = h;

		// We need to fill in enough members in 'h' to be able to create the arrays below. Note: We
		// know that 'Value' is simple enough to be copied with memcpy, so it is fine to have a
		// partly initialized handle for 'Value' for a small while.
		if (val) {
			const GcType *g = gcType();
			h->size = g->stride;
			h->locationHash = false;
			h->gcArrayType = g;
			h->toSFn = &defToS;
			handleToS = toSMissing;
		} else {
			// Plain class.
			h->size = sizeof(void *);
			h->locationHash = false;
			h->gcArrayType = &pointerArrayType;
			h->copyFn = null; // Memcpy is OK.
			h->deepCopyFn = &objDeepCopy;
			h->toSFn = &objToS;
		}

		// Populate the handle for some types manually. Otherwise, we will not be able to boot
		// properly.
		populateHandle(this, h);


		Scope scope = engine.scope();

		Array<Value> *r = new (engine) Array<Value>(1, thisPtr(this));
		Array<Value> *rr = new (engine) Array<Value>(2, thisPtr(this));
		Array<Value> *vv = new (engine) Array<Value>(2, Value(this));

		// Fill in the rest of the members.
		if (val) {
			// Find constructor.
			if (Function *f = as<Function>(find(CTOR, rr, scope)))
				updateHandle(f);

			// Find destructor.
			if (Function *f = as<Function>(find(DTOR, r, scope)))
				updateHandle(f);

			// Find deepCopy.
			if (Function *f = deepCopyFn())
				updateHandle(f);

			updateHandleToS(true, null);
		}

		// Find hash function.
		if (Function *f = as<Function>(find(S("hash"), r, scope)))
			updateHandle(f);

		// Find equal function.
		if (Function *f = as<Function>(find(S("=="), vv, scope)))
			updateHandle(f);

		// Find less-than function.
		if (Function *f = as<Function>(find(S("<"), vv, scope)))
			updateHandle(f);
	}

	void Type::updateHandleToS(bool first, Function *newFn) {
		if (!value())
			return;

		if (newFn) {
			if (*newFn->name != S("<<"))
				return;

			if (newFn->params->count() != 2)
				return;

			Type *strBufT = StrBuf::stormType(engine);
			if (newFn->params->at(0) != Value(strBufT))
				return;

			if (newFn->params->at(1).type != this)
				return;

			RefHandle *h = (RefHandle *)tHandle;
			h->setToS(newFn);
			handleToS = toSFound;

			// TODO: Remove the notification now?

			return;
		}

		if (!first && handleToS == toSFound)
			return;

		if (first || handleToS == toSNoParent) {
			handleToS = toSMissing;

			RefHandle *h = (RefHandle *)tHandle;

			if (parentLookup) {
				Array<Value> *bv = new (engine) Array<Value>(2, Value(this));
				Type *strBufT = StrBuf::stormType(engine);
				bv->at(0) = Value(strBufT);

				Scope s = engine.scope().child(parentLookup);
				if (Function *f = as<Function>(s.find(S("<<"), bv))) {
					h->setToS(f);
					handleToS = toSFound;
				}
			} else {
				handleToS = toSNoParent;
			}

			if (first && handleToS != toSFound) {
				Array<Value> *bv = new (engine) Array<Value>(2, Value(this));
				Type *strBufT = StrBuf::stormType(engine);
				bv->at(0) = thisPtr(strBufT);

				if (Function *f = as<Function>(strBufT->find(S("<<"), bv, engine.scope()))) {
					h->setToS(f);
					handleToS = toSFound;
				} else {
					// Probably too early for this... Wait for notifications from StrBuf.
					strBufT->watchAdd(this);
				}
			}

			if (handleToS == toSMissing) {
				// Find our parent...
				NameSet *firstSet = null;
				for (NameLookup *at = parent(); at != null && firstSet == null; at = at->parentLookup)
					firstSet = as<NameSet>(at);

				if (!firstSet)
					return;

				// Ask our parent to keep an eye out for us!
				firstSet->watchAdd(this);
			}
		}
	}

	void Type::updateHandle(Function *fn) {
		bool val = value();
		RefHandle *h = (RefHandle *)tHandle;
		Array<Value> *params = fn->params;

		if (params->count() < 1)
			return;
		bool refThis = params->at(0).ref;

		// Do not add constructor, destructor or deepCopy to the handle if this is a built-in type,
		// as that allows containers etc to use raw memcpy which is more efficient in many cases.
		// Also: it prevents infinite loops during startup due to recursive dependencies.
		bool userType = as<code::PrimitiveDesc>(typeDesc()) == null;
		Str *name = fn->name;
		if (val && *name == CTOR) {
			if (refThis && params->count() == 2 && params->at(1) == Value(this, true) && userType)
				h->setCopyCtor(fn->ref());
		} else if (val && *name == DTOR) {
			if (refThis && params->count() == 1 && userType)
				h->setDestroy(fn->ref());
		} else if (val && *name == S("deepCopy") && userType) {
			if (refThis && params->count() == 2 && params->at(1) == Value(CloneEnv::stormType(engine)))
				h->setDeepCopy(fn->ref());
		} else if (*name == S("hash")) {
			if (params->count() == 1) {
				if (allRefParams(fn))
					h->setHash(fn->ref());
				else
					h->hashFn = (Handle::HashFn)makeRefParams(fn);
			}
		} else if (*name == S("==")) {
			if (params->count() == 2) {
				if (allRefParams(fn))
					h->setEqual(fn->ref());
				else
					h->equalFn = (Handle::EqualFn)makeRefParams(fn);
			}
		} else if (*name == S("<")) {
			if (params->count() == 2) {
				if (allRefParams(fn))
					h->setLess(fn->ref());
				else
					h->lessFn = (Handle::LessFn)makeRefParams(fn);
			}
		}
	}

	Named *Type::findHere(SimplePart *part, Scope source) {
		return NameSet::find(part, source);
	}

	Named *Type::tryFindHere(SimplePart *part, Scope source) {
		return NameSet::tryFind(part, source);
	}

	void Type::toS(StrBuf *to) const {
		if (typeFlags & typeRawPtr) {
			*to << S("class (raw ptr) ");
		} else if (value()) {
			*to << S("value ");
		} else {
			*to << S("class ");
		}

		if (params)
			*to << new (this) SimplePart(name, params);
		else
			*to << new (this) SimplePart(name);

		if (chain != null && chain->super() != null) {
			if (chain->super() == TObject::stormType(engine)) {
			} else if (chain->super() == Object::stormType(engine)) {
			} else {
				*to << S(" extends ") << chain->super()->identifier();
				return;
			}
		}

		if (useThread) {
			*to << S(" on ") << useThread->identifier();
		}

		*to << S(" [");
		putVisibility(to);
		*to << S("]");
	}

	Function *Type::defaultCtor() {
		return as<Function>(find(CTOR, new (this) Array<Value>(1, thisPtr(this)), engine.scope()));
	}

	Function *Type::copyCtor() {
		return as<Function>(find(CTOR, new (this) Array<Value>(2, thisPtr(this)), engine.scope()));
	}

	Function *Type::assignFn() {
		return as<Function>(find(S("="), new (this) Array<Value>(2, thisPtr(this)), engine.scope()));
	}

	Function *Type::deepCopyFn() {
		Array<Value> *params = valList(engine, 2, thisPtr(this), Value(CloneEnv::stormType(engine)));
		return as<Function>(find(S("deepCopy"), params, engine.scope()));
	}

	Function *Type::destructor() {
		return as<Function>(find(DTOR, new (this) Array<Value>(1, thisPtr(this)), engine.scope()));
	}

	Function *Type::readRefFn() {
		if (readRef)
			return readRef;

		if (!value()) {
			// We can just use the default implementation shared between all objects.
			if (isA(TObject::stormType(engine)))
				return engine.readTObjFn();
			else
				return engine.readObjFn();
		}

		// Otherwise, we need to create our own function. Note: We're not actually adding the
		// function to the name tree. We want it to be invisible!
		Value r(this);
		code::Listing *src = new (engine) code::Listing(false, typeDesc());
		code::Var param = src->createParam(engine.ptrDesc());

		*src << code::prolog();
		*src << code::fnRetRef(param);

		readRef = dynamicFunction(engine, r, S("_read_"), valList(engine, 1, r.asRef()), src);
		readRef->parentLookup = engine.package();
		return readRef;
	}

	void *Type::operator new(size_t size, Engine &e, GcType *type) {
		assert(size <= type->stride, L"Invalid type description found!");
		return e.gc.alloc(type);
	}

	void Type::operator delete(void *mem, Engine &e, GcType *type) {}

	RootObject *alloc(Type *t) {
		Function *ctor = t->defaultCtor();
		if (!ctor)
			throw InternalError(L"Can not create " + ::toS(t->identifier()) + L", no default constructor.");
		void *data = runtime::allocObject(t->size().current(), t);
		typedef void *(*Fn)(void *);
		Fn fn = (Fn)ctor->ref().address();
		(*fn)(data);
		return (RootObject *)data;
	}


	/**
	 * VTable logic.
	 *
	 * Note: there is a small optimization we do here. If we know we are the leaf class for a
	 * specific function, that function only needs to be registered in the vtable but does not need
	 * to use vtable dispatch!
	 */

	void Type::vtableFnAdded(Function *fn) {
		// If we are a value, we do not have a vtable.
		if (value() || rawPtr())
			return;

		bool abstractFn = (fn->flags & namedAbstract) == namedAbstract;
		if (abstractFn)
			invalidateAbstract();

		// Try to find a function which overrides this function, or a function which we override. If
		// we found the function in either direction, we do not need to search in the other
		// direction, as they already are in the vtable in that case.
		//
		// Furthermore, if 'fn' is abstract, we insert it anyway since we're certain it will be used
		// eventually. Furthermore, it greatly simplifies (and speeds up) the implementation of 'abstract()'.
		OverridePart *part = new (engine) OverridePart(fn);
		if (vtableInsertSuper(part, fn) ||
			vtableInsertSubclasses(part, fn) ||
			abstractFn) {

			myVTable->insert(fn);
		}
	}

	Bool Type::vtableInsertSuper(OverridePart *fn, Named *original) {
		// See if our parent contains an appropriate function.
		Type *s = super();
		if (!s)
			return false;

		Function *found = as<Function>(s->findHere(fn, Scope()));
		if (found && found->visibleFrom(original)) {
			// Found it, no need to search further as all possible parent functions are in the
			// vtable already.
			s->myVTable->insert(found);
			return true;
		} else {
			return s->vtableInsertSuper(fn, original);
		}
	}

	Bool Type::vtableInsertSubclasses(OverridePart *fn, Named *original) {
		bool inserted = false;

		TypeChain::Iter i = chain->children();
		while (Type *child = i.next()) {
			// Note: We do not want to eagerly load the child class now. The VTable for the child is
			// not needed until it is instantiated anyway, and we will get notified when that happens.
			Function *found = as<Function>(child->tryFindHere(fn, Scope()));
			if (found && original->visibleFrom(found)) {
				// Found something. Insert it in the vtable. We do not need to go further down this
				// particular path as any overriding functions there are already found by now.
				child->myVTable->insert(found);
				inserted = true;
			} else {
				inserted |= child->vtableInsertSubclasses(fn, original);
			}
		}

		return inserted;
	}

	void Type::vtableNewSuper() {
		// Insert all functions in here and in our super-classes.
		for (Iter i = begin(), e = end(); i != e; ++i) {
			Function *f = as<Function>(i.v());
			if (!f)
				continue;
			if (*f->name == CTOR)
				continue;

			vtableFnAdded(f);
		}

		isAbstract = abstractUnknown;
	}

	void Type::vtableDetachedSuper(Type *old) {
		if (!old)
			return;

		if (!engine.has(bootTemplates))
			return;

		old->myVTable->removeChild(this);
		isAbstract = abstractUnknown;
	}

	void Type::invalidateAbstract() {
		isAbstract = abstractUnknown;

		TypeChain::Iter i = chain->children();
		while (Type *child = i.next())
			child->invalidateAbstract();
	}

}
