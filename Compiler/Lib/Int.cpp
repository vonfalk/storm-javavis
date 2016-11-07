#include "stdafx.h"
#include "Int.h"
#include "Core/Array.h"
#include "Core/Hash.h"
#include "Core/Str.h"
#include "Function.h"
#include "Number.h"

namespace storm {
	using namespace code;

	Type *createInt(Str *name, Size size, GcType *type) {
		return new (name) IntType(name, type);
	}

	IntType::IntType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sInt, type, null) {}

	Bool IntType::loadAll() {
		Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);
		Value b(StormInfo<Bool>::type(engine));

		add(inlinedFunction(engine, Value(this), L"+", vv, fnPtr(engine, &numAdd)));
		add(inlinedFunction(engine, Value(this), L"-", vv, fnPtr(engine, &numSub)));
		add(inlinedFunction(engine, Value(this), L"*", vv, fnPtr(engine, &numMul)));
		add(inlinedFunction(engine, Value(this), L"/", vv, fnPtr(engine, &numIDiv)));
		add(inlinedFunction(engine, Value(this), L"%", vv, fnPtr(engine, &numIMod)));

		add(inlinedFunction(engine, b, L"==", vv, fnPtr(engine, &numCmp<ifEqual>)));
		add(inlinedFunction(engine, b, L"!=", vv, fnPtr(engine, &numCmp<ifNotEqual>)));
		add(inlinedFunction(engine, b, L"<=", vv, fnPtr(engine, &numCmp<ifLessEqual>)));
		add(inlinedFunction(engine, b, L">=", vv, fnPtr(engine, &numCmp<ifGreaterEqual>)));
		add(inlinedFunction(engine, b, L"<", vv, fnPtr(engine, &numCmp<ifLess>)));
		add(inlinedFunction(engine, b, L">", vv, fnPtr(engine, &numCmp<ifGreater>)));

		add(inlinedFunction(engine, b, L"*++", r, fnPtr(engine, &numPostfixInc<Int>)));
		add(inlinedFunction(engine, b, L"++*", r, fnPtr(engine, &numPrefixInc<Int>)));
		add(inlinedFunction(engine, b, L"*--", r, fnPtr(engine, &numPostfixDec<Int>)));
		add(inlinedFunction(engine, b, L"--*", r, fnPtr(engine, &numPrefixDec<Int>)));

		add(inlinedFunction(engine, Value(this, true), L"=", rv, fnPtr(engine, &numAssign<Int>)));
		add(inlinedFunction(engine, Value(this), L"+=", rv, fnPtr(engine, &numInc<Int>)));
		add(inlinedFunction(engine, Value(this), L"-=", rv, fnPtr(engine, &numDec<Int>)));
		add(inlinedFunction(engine, Value(this), L"*=", rv, fnPtr(engine, &numScale<Int>)));
		add(inlinedFunction(engine, Value(this), L"/=", rv, fnPtr(engine, &numIDivScale<Int>)));
		add(inlinedFunction(engine, Value(this), L"%=", rv, fnPtr(engine, &numIModEq<Int>)));

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Int>)));
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Int>)));

		add(inlinedFunction(engine, Value(StormInfo<Nat>::type(engine)), L"nat", v, fnPtr(engine, &icast)));
		add(inlinedFunction(engine, Value(StormInfo<Byte>::type(engine)), L"byte", v, fnPtr(engine, &icast)));
		add(inlinedFunction(engine, Value(StormInfo<Long>::type(engine)), L"long", v, fnPtr(engine, &icast)));
		add(inlinedFunction(engine, Value(StormInfo<Word>::type(engine)), L"word", v, fnPtr(engine, &icast)));
		add(inlinedFunction(engine, Value(StormInfo<Float>::type(engine)), L"float", v, fnPtr(engine, &numToFloat)));

		add(nativeFunction(engine, Value(this), L"hash", v, &intHash));
		add(nativeFunction(engine, Value(this), L"min", vv, address(&numMin<Int>)));
		add(nativeFunction(engine, Value(this), L"max", vv, address(&numMax<Int>)));

		return Type::loadAll();
	}


	Type *createNat(Str *name, Size size, GcType *type) {
		return new (name) NatType(name, type);
	}

	NatType::NatType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sNat, type, null) {}

	Bool NatType::loadAll() {
		Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);
		Value b(StormInfo<Bool>::type(engine));

		add(inlinedFunction(engine, Value(this), L"+", vv, fnPtr(engine, &numAdd)));
		add(inlinedFunction(engine, Value(this), L"-", vv, fnPtr(engine, &numSub)));
		add(inlinedFunction(engine, Value(this), L"*", vv, fnPtr(engine, &numMul)));
		add(inlinedFunction(engine, Value(this), L"/", vv, fnPtr(engine, &numUDiv)));
		add(inlinedFunction(engine, Value(this), L"%", vv, fnPtr(engine, &numUMod)));

		add(inlinedFunction(engine, b, L"==", vv, fnPtr(engine, &numCmp<ifEqual>)));
		add(inlinedFunction(engine, b, L"!=", vv, fnPtr(engine, &numCmp<ifNotEqual>)));
		add(inlinedFunction(engine, b, L"<=", vv, fnPtr(engine, &numCmp<ifBelowEqual>)));
		add(inlinedFunction(engine, b, L">=", vv, fnPtr(engine, &numCmp<ifAboveEqual>)));
		add(inlinedFunction(engine, b, L"<", vv, fnPtr(engine, &numCmp<ifBelow>)));
		add(inlinedFunction(engine, b, L">", vv, fnPtr(engine, &numCmp<ifAbove>)));

		add(inlinedFunction(engine, b, L"*++", r, fnPtr(engine, &numPostfixInc<Nat>)));
		add(inlinedFunction(engine, b, L"++*", r, fnPtr(engine, &numPrefixInc<Nat>)));
		add(inlinedFunction(engine, b, L"*--", r, fnPtr(engine, &numPostfixDec<Nat>)));
		add(inlinedFunction(engine, b, L"--*", r, fnPtr(engine, &numPrefixDec<Nat>)));

		add(inlinedFunction(engine, Value(this, true), L"=", rv, fnPtr(engine, &numAssign<Nat>)));
		add(inlinedFunction(engine, Value(this), L"+=", rv, fnPtr(engine, &numInc<Nat>)));
		add(inlinedFunction(engine, Value(this), L"-=", rv, fnPtr(engine, &numDec<Nat>)));
		add(inlinedFunction(engine, Value(this), L"*=", rv, fnPtr(engine, &numScale<Nat>)));
		add(inlinedFunction(engine, Value(this), L"/=", rv, fnPtr(engine, &numUDivScale<Nat>)));
		add(inlinedFunction(engine, Value(this), L"%=", rv, fnPtr(engine, &numUModEq<Nat>)));

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Nat>)));
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Nat>)));

		add(inlinedFunction(engine, Value(StormInfo<Int>::type(engine)), L"int", v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Byte>::type(engine)), L"byte", v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Long>::type(engine)), L"long", v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Word>::type(engine)), L"word", v, fnPtr(engine, &ucast)));

		add(nativeFunction(engine, Value(this), L"hash", v, &natHash));
		add(nativeFunction(engine, Value(this), L"min", vv, address(&numMin<Nat>)));
		add(nativeFunction(engine, Value(this), L"max", vv, address(&numMax<Nat>)));

		return Type::loadAll();
	}

}
