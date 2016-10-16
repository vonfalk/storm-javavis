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
		NameSet(name), engine(RootObject::engine()), gcType(null), tHandle(null), typeFlags(flags & ~typeCpp) {

		init();
	}

	Type::Type(Str *name, Array<Value> *params, TypeFlags flags) :
		NameSet(name, params), engine(RootObject::engine()), gcType(null), tHandle(null), typeFlags(flags & ~typeCpp) {

		init();
	}

	Type::Type(Str *name, TypeFlags flags, Size size, GcType *gcType) :
		NameSet(name), engine(RootObject::engine()), gcType(gcType), tHandle(null), typeFlags(flags | typeCpp), mySize(size) {

		gcType->type = this;
		init();
	}

	Type::Type(Str *name, Array<Value> *params, TypeFlags flags, Size size, GcType *gcType) :
		NameSet(name, params), engine(RootObject::engine()), gcType(gcType), tHandle(null), typeFlags(flags | typeCpp), mySize(size) {

		gcType->type = this;
		init();
	}

	// We need to set gcType->type first, therefore we call setMyType!
	Type::Type(Engine &e, TypeFlags flags, Size size, GcType *gcType) :
		NameSet(setMyType(null, this, gcType, e)), engine(e), gcType(gcType),
		tHandle(null), typeFlags(typeClass | typeCpp) {

		init();
	}

	Type::~Type() {
		GcType *g = gcType;
		gcType = null;
		// Barrier here?

		// The GC will ignore these during shutdown, when it is crucial not to destroy anything too
		// early.
		// NOTE: Destroying types here is generally a bad idea, as non-reachable objects may still
		// reside on the heap at this point.
		engine.gc.freeType(g);
	}

	void Type::init() {
		assert((typeFlags & typeValue) == typeValue || (typeFlags & typeClass) == typeClass, L"Invalid type flags!");

		if (value()) {
			gcType->kind = GcType::tArray;
		}

		if (engine.has(bootTemplates))
			lateInit();
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

	Size Type::superSize() {
		if (super())
			return super()->size();

		if (value())
			return Size();

		assert(false, L"We are a class which does not inherit from TObject or Object!");
		return Size();
	}

	Size Type::size() {
		if (mySize == Size()) {
			// Re-compute! We need to switch threads...
			const os::Thread &t = TObject::thread->thread();
			if (t == os::Thread::current()) {
				// Already on the compiler thread.
				forceLoad();
				Size s = superSize();
				TODO(L"Fixme!");
				// mySize = layout->size(s);
				mySize = s;
				if (value())
					TODO(L"Update the handle as well.");
			} else {
				// We need to run on the Compiler thread. Note the 'Semaphore', this is to ensure
				// that we do not break any semantics regarding the threading model.
				os::Future<Size, Semaphore> f;
				os::FnParams p; p.add(this);
				os::UThread::spawn(address(&Type::size), true, p, f, &t);
				return f.result();
			}
		}
		return mySize;
	}

	// Our finalizer.
	static void destroyType(Type *t) {
		t->~Type();
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

		// Insert 'gcType' as the first (special) pointer:
		t->offset[0] = OFFSET_OF(Type, gcType);
		for (nat i = 0; i < entries; i++)
			t->offset[i+1] = Offset(type->ptrOffsets[i]).current();

		// Ensure we're finalized.
		t->finalizer = address(&destroyType);

		// Now we can allocate the type and let the constructor handle the rest!
		return new (e, t) Type(e, typeClass, Size(type->size), t);
	}

	void Type::setType(Object *onto) const {
		engine.gc.switchType(onto, gcType);
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

		// For now, this is sufficient.
		if ((typeFlags & typeCpp) != typeCpp)
			gcType = engine.gc.allocType(to->gcType);
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

		if (value() && tHandle)
			if (Function *f = as<Function>(item))
				updateHandle(f);
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

	static void defToS(const void *obj, StrBuf *to) {
		*to << L"<operator << not found>";
	}

	const Handle &Type::handle() {
		if (!tHandle)
			buildHandle();
		if (handleToS != toSFound)
			updateHandleToS(false, null);
		return *tHandle;
	}

	const GcType *Type::gcArrayType() {
		if (value()) {
			return gcType;
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
			h->size = gcType->stride;
			h->gcArrayType = gcType;
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
