#include "stdafx.h"
#include "Type.h"
#include "Engine.h"
#include "Gen/CppTypes.h"

namespace storm {

	Type::Type(TypeFlags flags) : engine(Object::engine()), gcType(null) {}

	Type::Type(TypeFlags flags, Size size, GcType *gcType) : engine(Object::engine()), gcType(gcType) {
		gcType->type = this;
	}

	Type::Type(Engine &e, TypeFlags flags, Size size, GcType *gcType) : engine(e), gcType(gcType) {
		gcType->type = this;
	}

	Type::~Type() {
		if (gcType->kind == GcType::tType) {
			TODO(L"Free this after everything is properly shut down!");
		}
		GcType *g = gcType;
		gcType = null;
		// Barrier here?
		engine.gc.freeType(g);
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

		// Now we can allocate the type and let the constructor handle the rest!
		return new (e, t) Type(e, typeClass, Size(type->size), t);
	}

	void *Type::operator new(size_t size, Engine &e, GcType *type) {
		assert(size <= type->stride, L"Invalid type description found!");
		return e.gc.alloc(type);
	}

	void Type::operator delete(void *mem, Engine &e, GcType *type) {}

}
