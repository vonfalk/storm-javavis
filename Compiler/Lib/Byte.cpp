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

	static Nat CODECALL byteRefHash(Byte &b) {
		return byteHash(b);
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

		add(inlinedFunction(engine, Value(this), L"+", vv, fnPtr(engine, &numAdd)));
		add(inlinedFunction(engine, Value(this), L"-", vv, fnPtr(engine, &numSub)));

		// Not supported yet.
		// add(inlinedFunction(engine, Value(this), L"*", vv, fnPtr(engine, &numMul)));
		// add(inlinedFunction(engine, Value(this), L"/", vv, fnPtr(engine, &numIDiv)));
		// add(inlinedFunction(engine, Value(this), L"%", vv, fnPtr(engine, &numIMod)));

		add(inlinedFunction(engine, b, L"==", vv, fnPtr(engine, &numCmp<ifEqual>)));
		add(inlinedFunction(engine, b, L"!=", vv, fnPtr(engine, &numCmp<ifNotEqual>)));
		add(inlinedFunction(engine, b, L"<=", vv, fnPtr(engine, &numCmp<ifLessEqual>)));
		add(inlinedFunction(engine, b, L">=", vv, fnPtr(engine, &numCmp<ifGreaterEqual>)));
		add(inlinedFunction(engine, b, L"<", vv, fnPtr(engine, &numCmp<ifLess>)));
		add(inlinedFunction(engine, b, L">", vv, fnPtr(engine, &numCmp<ifGreater>)));

		add(inlinedFunction(engine, Value(this), L"*++", r, fnPtr(engine, &numPostfixInc<Byte>)));
		add(inlinedFunction(engine, Value(this), L"++*", r, fnPtr(engine, &numPrefixInc<Byte>)));
		add(inlinedFunction(engine, Value(this), L"*--", r, fnPtr(engine, &numPostfixDec<Byte>)));
		add(inlinedFunction(engine, Value(this), L"--*", r, fnPtr(engine, &numPrefixDec<Byte>)));

		add(inlinedFunction(engine, Value(this, true), L"=", rv, fnPtr(engine, &numAssign<Byte>)));
		add(inlinedFunction(engine, Value(this), L"+=", rv, fnPtr(engine, &numInc<Byte>)));
		add(inlinedFunction(engine, Value(this), L"-=", rv, fnPtr(engine, &numDec<Byte>)));

		// Not supported yet!
		// add(inlinedFunction(engine, Value(this), L"*=", rv, fnPtr(engine, &numScale<Byte>)));
		// add(inlinedFunction(engine, Value(this), L"/=", rv, fnPtr(engine, &numIDivScale<Byte>)));
		// add(inlinedFunction(engine, Value(this), L"%=", rv, fnPtr(engine, &numIModEq<Byte>)));

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Byte>)));
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Byte>)));

		add(inlinedFunction(engine, Value(StormInfo<Int>::type(engine)), L"int", v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Nat>::type(engine)), L"nat", v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Long>::type(engine)), L"long", v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Word>::type(engine)), L"word", v, fnPtr(engine, &ucast)));
		add(nativeFunction(engine, Value(StormInfo<Float>::type(engine)), L"float", v, address(&byteToFloat)));

		Value n(StormInfo<Nat>::type(engine));
		add(nativeFunction(engine, n, L"hash", r, &byteHash));
		add(nativeFunction(engine, Value(this), L"min", vv, address(&numMin<Byte>)));
		add(nativeFunction(engine, Value(this), L"max", vv, address(&numMax<Byte>)));

		return Type::loadAll();
	}

}
