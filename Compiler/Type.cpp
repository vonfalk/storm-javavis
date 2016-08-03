#include "stdafx.h"
#include "Type.h"
#include "Engine.h"
#include "Core/Handle.h"
#include "Core/Gen/CppTypes.h"

namespace storm {

	// Set 'type->type' to 'me' while forwarding 'name'. This has to be done before invoking the
	// parent constructor, since that relies on 'engine()' working properly, which is not the case
	// for the first object otherwise.
	static Str *setMyType(Str *name, Type *me, GcType *type, Engine &e) {
		// Set the type properly.
		type->type = me;

		// We need to set the engine as well. Should be the first member of this class.
		OFFSET_IN(me, sizeof(Named), Engine *) = &e;

		// Check to see if we succeeded!
		assert(&me->engine == &e, L"Type::engine must be the first data member declared in Type.");

		return name;
	}

	Type::Type(Str *name, TypeFlags flags) :
		Named(name), engine(Object::engine()), gcType(null), tHandle(null), typeFlags(flags) {

		init();
	}

	Type::Type(Str *name, TypeFlags flags, Size size, GcType *gcType) :
		Named(name), engine(Object::engine()), gcType(gcType), tHandle(null), typeFlags(flags) {

		gcType->type = this;
		init();
	}

	// We need to set gcType->type first, therefore we call setMyType!
	Type::Type(Engine &e, TypeFlags flags, Size size, GcType *gcType) :
		Named(setMyType(null, this, gcType, e)), engine(e), gcType(gcType), tHandle(null), typeFlags(typeClass) {
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
	}

	// Our finalizer.
	static void destroyType(Type *t) {
		t->~Type();
	}

	Type *Type::createType(Engine &e, const CppType *type) {
		assert(wcscmp(type->name, L"Type") == 0, L"storm::Type was not found!");
		assert(Size(type->size).current() == sizeof(Type),
			L"The computed size of storm::Type is wrong: " + ::toS(Size(type->size)) + L" vs " + ::toS(sizeof(Type)));

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

	const Handle &Type::handle() {
		if (!tHandle)
			tHandle = buildHandle();
		return *tHandle;
	}

	const Handle *Type::buildHandle() {
		if (value()) {
			Handle *h = new (engine) Handle();

			h->size = gcType->stride;
			h->gcArrayType = gcType;
			assert(false, L"TODO: look up copy ctors and so on!");
			return h;
		} else {
			// Standard pointer handle.
			return &engine.ptrHandle();
		}
	}

	void *Type::operator new(size_t size, Engine &e, GcType *type) {
		assert(size <= type->stride, L"Invalid type description found!");
		return e.gc.alloc(type);
	}

	void Type::operator delete(void *mem, Engine &e, GcType *type) {}

}
