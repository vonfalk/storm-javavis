#include "stdafx.h"
#include "Unknown.h"
#include "Compiler/Engine.h"

namespace storm {

	UnknownType::UnknownType(Str *name, Size size, GcType *type)
		: Type(name, typeValue | typeFinal, size, type, null) {}

	code::TypeDesc *UnknownType::createTypeDesc() {
		Size s = size();
		if (s == Size::sPtr) {
			return engine.ptrDesc();
		} else {
			return code::intDesc(engine);
		}
	}

	static void copyPtr(void **to, void **from) {
		*to = *from;
	}

	static void copyInt(Int *to, Int *from) {
		*to = *from;
	}

	Bool UnknownType::loadAll() {
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		// Note: These constructors do not need to be very efficient. They are almost never called.
		if (size() == Size::sPtr) {
			add(nativeFunction(engine, Value(), Type::CTOR, rr, address(&copyPtr))->makePure());
		} else {
			add(nativeFunction(engine, Value(), Type::CTOR, rr, address(&copyInt))->makePure());
		}

		return true;
	}

	Type *createUnknownInt(Str *name, Size size, GcType *type) {
		return new (name) UnknownType(name, Size::sInt, type);
	}

	Type *createUnknownGc(Str *name, Size size, GcType *type) {
		// The GcType here will probably not contain proper data for the type. Fix that!
		if (type->count == 0) {
			Engine &e = name->engine();
			GcType *old = type;
			type = e.gc.allocType(GcType::Kind(old->kind), old->type, old->stride, 1);
			e.gc.freeType(old);

			// We're a pointer!
			type->offset[0] = 0;
		}

		return new (name) UnknownType(name, Size::sPtr, type);
	}

	Type *createUnknownNoGc(Str *name, Size size, GcType *type) {
		return new (name) UnknownType(name, Size::sPtr, type);
	}

}
