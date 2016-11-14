#include "stdafx.h"
#include "Type.h"
#include "Engine.h"
#include "NamedThread.h"
#include "Function.h"
#include "Core/Str.h"
#include "Core/Handle.h"
#include "Core/Gen/CppTypes.h"
#include "Core/StrBuf.h"
#include "OS/UThread.h"
#include "OS/Future.h"
#include "Utils/Memory.h"

namespace storm {

	const wchar *Type::CTOR = L"__init";
	const wchar *Type::DTOR = L"__dtor";


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

		if (value()) {
			myGcType->kind = GcType::tArray;
		}

		if (engine.has(bootTypes))
			vtableInit(vtable);

		if (engine.has(bootTemplates))
			lateInit();
	}

	void Type::vtableInit(const void *vtab) {
		if (value())
			return;

		vtable = new (engine) VTable();
		if (typeFlags & typeCpp) {
			vtable->createCpp(vtab);
		} else if (Type *t = super()) {
			vtable->createStorm(t->vtable);
		} else {
			vtable->createStorm(Object::stormType(engine)->vtable);
		}
	}

	void Type::lateInit() {
		NameSet::lateInit();

		if (!chain) {
			chain = new (this) TypeChain(this);
			if (!value())
				setSuper(Object::stormType(engine));
		}

		chain->lateInit();
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
		assert(wcscmp(type->name, L"Type") == 0, L"storm::Type was not found!");
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

	void Type::setSuper(Type *to) {
		// So that Object does not inherit from TObject.
		if (to == this)
			return;

		if (!chain)
			chain = new (this) TypeChain(this);

		// Nothing to do?
		if (to == chain->super())
			return;

		if (!to->chain)
			to->chain = new (this) TypeChain(to);

		// Which thread to use?
		Type *tObj = TObject::stormType(engine);
		if (to->chain != null && tObj->chain != null) {
			if (!to->chain->isA(tObj)) {
				useThread = null;
			} else if (to != tObj) {
				useThread = to->useThread;
				assert(useThread, L"No thread on a threaded object!");
			}
		}

		// Set the type-chain properly.
		chain->super(to);

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
		// TODO: Notify our old parent of removal of vtable members.

		// Re-initialize the vtable.
		if ((typeFlags & typeCpp) != typeCpp && !value()) {
			vtable->createStorm(to->vtable);
		}

		// Register all functions here and in our children as vtable calls.
		vtableNewSuper();
	}

	void Type::setThread(NamedThread *thread) {
		useThread = thread;
		setSuper(TObject::stormType(engine));

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

	void Type::add(Named *item) {
		NameSet::add(item);

		if (Function *f = as<Function>(item)) {
			if (wcscmp(f->name->c_str(), CTOR) != 0)
				vtableFnAdded(f);

			if (value() && tHandle)
				updateHandle(f);
		}

		if (value() && tHandle)
			if (Function *f = as<Function>(item))
				updateHandle(f);

		if ((typeFlags & typeCpp) != typeCpp) {
			// Tell the layout we found a new variable!
			if (!layout)
				layout = new (engine) Layout();
			layout->add(item);
		}
	}

	Named *Type::find(SimplePart *part) {
		if (Named *n = NameSet::find(part))
			return n;

		// Constructors are not inherited.
		if (wcscmp(part->name->c_str(), CTOR) == 0)
			return null;

		if (Type *s = super())
			return s->find(part);
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

		if (chain != null && engine.has(bootTemplates)) {
			Array<Type *> *children = chain->children();
			for (nat i = 0; i < children->count(); i++) {
				children->at(i)->notifyThread(thread);
			}
		}
	}

	BasicTypeInfo::Kind Type::builtInType() const {
		return BasicTypeInfo::user;
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

	Size Type::superSize() {
		if (super())
			return super()->size();

		if (value())
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
			os::Future<Size, Semaphore> f;
			os::FnStackParams<2> p; p.add(this);
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

	static void defToS(const void *obj, StrBuf *to) {
		*to << L"<operator << not found>";
	}

	const Handle &Type::handle() {
		// If we're completely done with the handle, return it immediatly.
		if (tHandle && handleToS == toSFound)
			return *tHandle;

		// We need to create the handle. Switch threads.
		const os::Thread &t = TObject::thread->thread();
		if (t != os::Thread::current()) {
			// Switch threads...
			os::Future<const Handle *, Semaphore> f;
			os::FnStackParams<2> p; p.add(this);
			os::UThread::spawn(address(&Type::handle), true, p, f, &t);
			return *f.result();
		}

		if (!tHandle)
			buildHandle();
		if (handleToS != toSFound)
			updateHandleToS(false, null);
		return *tHandle;
	}

	const GcType *Type::gcType() {
		// Already created?
		if (myGcType != null)
			return myGcType;

		// We need to create it. Switch threads if neccessary...
		const os::Thread &t = TObject::thread->thread();
		if (t != os::Thread::current()) {
			// Wrong thread, switch!
			os::Future<const GcType *, Semaphore> f;
			os::FnStackParams<2> p; p.add(this);
			os::UThread::spawn(address(&Type::gcType), true, p, f, &t);
			return f.result();
		}

		// We're on the correct thread. Compute the type!
		assert((typeFlags & typeCpp) != typeCpp, L"C++ types should be given a GcType on creation!");

		// Compute the GcType for us!
		nat count = 0;
		const GcType *superGc = null;
		if (Type *s = super())
			superGc = s->gcType();

		if (!layout) {
			// We do not have any variables of our own.
			if (superGc)
				myGcType = engine.gc.allocType(superGc);
			else if (value())
				myGcType = engine.gc.allocType(GcType::tArray, this, size().current(), 0);
			else
				assert(false, L"We're a non-value not inheriting from Object or TObject!");

			return myGcType;
		}

		// Merge our parent's and our offsets (if we have a parent).
		nat entries = layout->fillGcType(superGc, null);

		if (value())
			myGcType = engine.gc.allocType(GcType::tArray, this, size().current(), entries);
		else if (superGc)
			myGcType = engine.gc.allocType(GcType::Kind(superGc->kind), this, size().current(), entries);
		else
			assert(false, L"Neither a value nor have a parent!");

		layout->fillGcType(superGc, myGcType);
		return myGcType;
	}

	const GcType *Type::gcArrayType() {
		if (value()) {
			return gcType();
		} else if (useThread) {
			return engine.tObjHandle().gcArrayType;
		} else {
			return engine.objHandle().gcArrayType;
		}
	}

	void Type::buildHandle() {
		if (value()) {
			if (!handleContent)
				handleContent = new (engine) code::Content();

			RefHandle *h = new (engine) RefHandle(handleContent);
			const GcType *g = gcType();
			h->size = g->stride;
			h->gcArrayType = g;
			h->toSFn = &defToS;
			handleToS = toSMissing;
			tHandle = h;

			Array<Value> *r = new (engine) Array<Value>(1, Value(this, true));
			Array<Value> *rr = new (engine) Array<Value>(2, Value(this, true));
			Array<Value> *rv = new (engine) Array<Value>(2, Value(this, true));
			rv->at(1) = Value(this);

			// Find constructor.
			if (Function *f = as<Function>(find(CTOR, rr)))
				updateHandle(f);

			// Find destructor.
			if (Function *f = as<Function>(find(DTOR, r)))
				updateHandle(f);

			{
				// Find deepCopy.
				Array<Value> *rc = new (engine) Array<Value>(2, Value(this, true));
				rc->at(1) = Value(CloneEnv::stormType(engine));
				if (Function *f = as<Function>(find(L"deepCopy", rc)))
					updateHandle(f);
			}

			updateHandleToS(true, null);

			// Find hash function.
			if (Function *f = as<Function>(find(L"hash", r)))
				updateHandle(f);

			// Find equal function.
			if (Function *f = as<Function>(find(L"equal", rv)))
				updateHandle(f);

		} else if (useThread) {
			// Standard tObject handle.
			tHandle = &engine.tObjHandle();
		} else {
			// Standard pointer handle.
			tHandle = &engine.objHandle();
		}
	}

	void Type::updateHandleToS(bool first, Function *newFn) {
		if (!value())
			return;

		if (newFn) {
			if (wcscmp(newFn->name->c_str(), L"<<") != 0)
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
				if (Function *f = as<Function>(s.find(L"<<", bv))) {
					h->setToS(f);
					handleToS = toSFound;
				}
			} else {
				handleToS = toSNoParent;
			}

			if (first && handleToS != toSFound) {
				Array<Value> *bv = new (engine) Array<Value>(2, Value(this));
				Type *strBufT = StrBuf::stormType(engine);
				bv->at(0) = Value(strBufT);

				if (Function *f = as<Function>(strBufT->find(L"<<", bv))) {
					h->setToS(f);
					handleToS = toSFound;
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
		assert(value());
		RefHandle *h = (RefHandle *)tHandle;

		if (fn->params->count() < 1)
			return;
		if (fn->params->at(0) != Value(this, true))
			return;

		const wchar *name = fn->name->c_str();
		if (wcscmp(name, CTOR) == 0) {
			if (params->count() == 2 && params->at(1) == Value(this, true))
				h->setCopyCtor(fn->ref());
		} else if (wcscmp(name, DTOR) == 0) {
			if (params->count() == 1)
				h->setDestroy(fn->ref());
		} else if (wcscmp(name, L"deepCopy") == 0) {
			if (params->count() == 2 && params->at(1) == Value(CloneEnv::stormType(engine)))
				h->setDeepCopy(fn->ref());
		} else if (wcscmp(name, L"hash") == 0) {
			if (params->count() == 1)
				h->setHash(fn->ref());
		} else if (wcscmp(name, L"equal") == 0) {
			if (params->count() == 1)
				h->setEqual(fn->ref());
		}
	}

	Named *Type::findHere(SimplePart *part) {
		return NameSet::find(part);
	}

	/**
	 * VTable logic.
	 *
	 * Note: there is a small optimization we can do here. If we know we are the leaf class for a
	 * specific function, that function only needs to be registered in the vtable but does not need
	 * to use vtable dispatch!
	 */

	void Type::vtableFnAdded(Function *fn) {
		if (!vtableFindSuper(fn))
			vtableFindSubclass(new (engine) OverridePart(fn));
	}

	Bool Type::vtableFindSuper(Function *fn) {
		// Ask our parent to see if 'fn' should be virtual.
		VTableSlot slot;
		if (Type *s = super())
			slot = s->vtableNewChildFn(new (engine) OverridePart(fn));

		if (slot.valid()) {
			// NOTE: We do not need to set vtable use, as we're still the leaf function.
			vtableInsert(fn, slot);
			return true;
		}

		return false;
	}

	// TODO: We can optimize this a whole lot by elliminating the need to actually find the parent
	// function again.
	void Type::vtableFindSubclass(OverridePart *fn) {
		// If too early in the boot process, we can not proceed here.
		if (!engine.has(bootTemplates))
			return;

		Array<Type *> *children = chain->children();
		for (Nat i = 0; i < children->count(); i++) {
			Type *child = children->at(i);

			Function *found = as<Function>(child->findHere(fn));
			if (found) {
				// Find and apply the vtable layout.
				// TODO: Do we need to force-disable the vtable?
				child->vtableFindSuper(found);
			}

			child->vtableFindSubclass(fn);
		}
	}

	VTableSlot Type::vtableNewChildFn(OverridePart *fn) {
		Function *found = as<Function>(findHere(fn));
		if (!found) {
			if (Type *s = super())
				return s->vtableNewChildFn(fn);
			else
				return VTableSlot();
		}

		// See if 'found' already has a VTable slot, otherwise allocate one.
		VTableSlot slot = vtable->findSlot(found);
		if (!slot.valid()) {
			// Allocate a new slot...
			TODO(L"FIXME!");
			return VTableSlot();
			// Insert this function into that slot
			vtableInsert(found, slot);
		}

		// We need to use the vtable.
		vtableUse(found, slot);
		return slot;
	}

	void Type::vtableUse(Function *fn, VTableSlot slot) {
		// PLN(fn->identifier() << L" should be virtual using " << slot);
		code::RefSource *src = engine.vtableCalls()->get(slot);
		fn->setLookup(new (engine) DelegatedCode(code::Ref(src)));
	}

	void Type::vtableInsert(Function *fn, VTableSlot slot) {
		vtable->set(slot, fn, handleContent);
	}

	void Type::vtableClear(Function *fn) {
		// PLN(fn->identifier() << L" should not be virtual.");
		fn->setLookup(null);
	}

	void Type::vtableNewSuper() {
		// If too early in the boot process, we can not proceed here.
		if (!engine.has(bootTemplates))
			return;

		// Clear the vtable usage for all functions in here and pretend they were added once more to
		// properly register them with the super class.
		for (NameSet::Iter i = begin(), e = end(); i != e; ++i) {
			Function *f = as<Function>(i.v());
			vtableClear(f);
			vtableFindSuper(f);
		}

		Array<Type *> *children = chain->children();
		for (Nat i = 0; i < children->count(); i++) {
			children->at(i)->vtableNewSuper();
		}
	}

	// NOTE: Slightly dangerous to re-use the parameters from the function...
	OverridePart::OverridePart(Function *src) :	SimplePart(src->name, src->params), result(src->result) {}

	Int OverridePart::matches(Named *candidate) const {
		Function *fn = as<Function>(candidate);
		if (!fn)
			return -1;

		Array<Value> *c = fn->params;
		if (c->count() != params->count())
			return -1;

		if (c->count() < 1)
			return -1;

		if (params->at(0).canStore(c->at(0))) {
			// Candidate is in a subclass wrt us.
			for (nat i = 1; i < c->count(); i++) {
				// Candidate needs to accept wider inputs than us.
				if (!c->at(i).canStore(params->at(i)))
					return -1;
			}

			// Candidate may return a narrower range of types.
			if (!fn->result.canStore(result))
				return -1;

		} else if (c->at(0).canStore(params->at(0))) {
			// Candidate is in a superclass wrt us.
			for (nat i = 1; i < c->count(); i++) {
				// We need to accept wider inputs than candidate.
				if (!params->at(i).canStore(c->at(i)))
					return -1;
			}

			// We may return a narrower range than candidate.
			if (!result.canStore(fn->result))
				return -1;

		} else {
			return -1;
		}

		// We always give a binary decision.
		return 0;
	}

	void Type::toS(StrBuf *to) const {
		if (value()) {
			*to << L"value ";
		} else {
			*to << L"class ";
		}

		if (params)
			*to << new (this) SimplePart(name, params);
		else
			*to << new (this) SimplePart(name);

		if (useThread) {
			*to << L" on " << useThread->identifier();
		} else if (chain != null && chain->super() != null) {
			*to << L" extends " << chain->super()->identifier();
		}
	}

	void *Type::operator new(size_t size, Engine &e, GcType *type) {
		assert(size <= type->stride, L"Invalid type description found!");
		return e.gc.alloc(type);
	}

	void Type::operator delete(void *mem, Engine &e, GcType *type) {}

}
