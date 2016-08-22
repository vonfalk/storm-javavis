#include "stdafx.h"
#include "Type.h"
#include "Engine.h"
#include "NamedThread.h"
#include "Core/Str.h"
#include "Core/Handle.h"
#include "Core/Gen/CppTypes.h"
#include "Core/StrBuf.h"
#include "Utils/Memory.h"

namespace storm {

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
		NameSet(name), engine(RootObject::engine()), gcType(gcType), tHandle(null), typeFlags(flags | typeCpp) {

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

	void Type::notifyThread(NamedThread *thread) {
		useThread = thread;

		if (chain != null && engine.has(bootTemplates)) {
			Array<Type *> *children = chain->children();
			for (nat i = 0; i < children->count(); i++) {
				children->at(i)->notifyThread(thread);
			}
		}
	}

	const Handle &Type::handle() {
		if (!tHandle)
			tHandle = buildHandle();
		return *tHandle;
	}

	static void defToS(const void *obj, StrBuf *to) {
		*to << L"<unknown>";
	}

	const Handle *Type::buildHandle() {
		if (value()) {
			Handle *h = new (engine) Handle();

			h->size = gcType->stride;
			h->gcArrayType = gcType;
			h->toSFn = &defToS;
			TODO(L"Look up copy ctors and so on!");
			// For now: we do not have anything that needs special care when being copied, so we're
			// fine with the defaults.
			return h;
		} else if (useThread) {
			// Standard tObject handle.
			return &engine.tObjHandle();
		} else {
			// Standard pointer handle.
			return &engine.objHandle();
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
