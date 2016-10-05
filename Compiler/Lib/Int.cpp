#include "stdafx.h"
#include "Int.h"
#include "Core/Array.h"
#include "Core/Hash.h"
#include "Core/Str.h"
#include "Function.h"
#include "Number.h"

namespace storm {

	Type *createInt(Str *name, Size size, GcType *type) {
		return new (name) IntType(name, type);
	}

	IntType::IntType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sInt, type) {}

	Bool IntType::loadAll() {
		Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));

		add(nativeFunction(engine, Value(this), L"hash", v, &intHash));
		add(nativeFunction(engine, Value(this), L"min", vv, address(&numMin<Int>)));
		add(nativeFunction(engine, Value(this), L"max", vv, address(&numMax<Int>)));

		return Type::loadAll();
	}

}
