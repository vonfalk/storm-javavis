#include "stdafx.h"
#include "Byte.h"
#include "Core/Array.h"
#include "Core/Hash.h"
#include "Core/Str.h"
#include "Function.h"
#include "Number.h"

namespace storm {
	using namespace code;

	static float CODECALL byteToFloat(Byte b) {
		return float(b);
	}

	Type *createByte(Str *name, Size size, GcType *type) {
		return new (name) ByteType(name, type);
	}

	ByteType::ByteType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sByte, type, null) {}

	Bool ByteType::loadAll() {
		Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);
		Value b(StormInfo<Bool>::type(engine));

		add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &numAdd)));
		add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &numSub)));

		// Not supported yet.
		// add(inlinedFunction(engine, Value(this), S("*"), vv, fnPtr(engine, &numMul)));
		// add(inlinedFunction(engine, Value(this), S("/"), vv, fnPtr(engine, &numIDiv)));
		// add(inlinedFunction(engine, Value(this), S("%"), vv, fnPtr(engine, &numIMod)));

		add(inlinedFunction(engine, b, S("=="), vv, fnPtr(engine, &numCmp<ifEqual>)));
		add(inlinedFunction(engine, b, S("!="), vv, fnPtr(engine, &numCmp<ifNotEqual>)));
		add(inlinedFunction(engine, b, S("<="), vv, fnPtr(engine, &numCmp<ifLessEqual>)));
		add(inlinedFunction(engine, b, S(">="), vv, fnPtr(engine, &numCmp<ifGreaterEqual>)));
		add(inlinedFunction(engine, b, S("<"), vv, fnPtr(engine, &numCmp<ifLess>)));
		add(inlinedFunction(engine, b, S(">"), vv, fnPtr(engine, &numCmp<ifGreater>)));

		add(inlinedFunction(engine, Value(this), S("*++"), r, fnPtr(engine, &numPostfixInc<Byte>)));
		add(inlinedFunction(engine, Value(this), S("++*"), r, fnPtr(engine, &numPrefixInc<Byte>)));
		add(inlinedFunction(engine, Value(this), S("*--"), r, fnPtr(engine, &numPostfixDec<Byte>)));
		add(inlinedFunction(engine, Value(this), S("--*"), r, fnPtr(engine, &numPrefixDec<Byte>)));

		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &numAssign<Byte>)));
		add(inlinedFunction(engine, Value(this), S("+="), rv, fnPtr(engine, &numInc<Byte>)));
		add(inlinedFunction(engine, Value(this), S("-="), rv, fnPtr(engine, &numDec<Byte>)));

		// Not supported yet!
		// add(inlinedFunction(engine, Value(this), S("*="), rv, fnPtr(engine, &numScale<Byte>)));
		// add(inlinedFunction(engine, Value(this), S("/="), rv, fnPtr(engine, &numIDivScale<Byte>)));
		// add(inlinedFunction(engine, Value(this), S("%="), rv, fnPtr(engine, &numIModEq<Byte>)));

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Byte>)));
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Byte>)));

		add(inlinedFunction(engine, Value(StormInfo<Int>::type(engine)), S("int"), v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Nat>::type(engine)), S("nat"), v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Long>::type(engine)), S("long"), v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Word>::type(engine)), S("word"), v, fnPtr(engine, &ucast)));
		add(nativeFunction(engine, Value(StormInfo<Float>::type(engine)), S("float"), v, address(&byteToFloat)));

		Value n(StormInfo<Nat>::type(engine));
		add(nativeFunction(engine, n, S("hash"), v, address(&byteHash)));
		add(inlinedFunction(engine, Value(this), S("min"), vv, fnPtr(engine, &numMin<Byte>)));
		add(inlinedFunction(engine, Value(this), S("max"), vv, fnPtr(engine, &numMax<Byte>)));
		add(inlinedFunction(engine, Value(this), S("delta"), vv, fnPtr(engine, &numDelta<Byte>)));

		return Type::loadAll();
	}

}
