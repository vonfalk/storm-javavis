#include "stdafx.h"
#include "Long.h"
#include "Core/Array.h"
#include "Core/Hash.h"
#include "Core/Str.h"
#include "Function.h"
#include "Number.h"

namespace storm {
	using namespace code;

	Type *createLong(Str *name, Size size, GcType *type) {
		return new (name) LongType(name, type);
	}

	LongType::LongType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sLong, type, null) {}

	Bool LongType::loadAll() {
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

		add(inlinedFunction(engine, Value(this), L"*++", r, fnPtr(engine, &numPostfixInc<Long>)));
		add(inlinedFunction(engine, Value(this), L"++*", r, fnPtr(engine, &numPrefixInc<Long>)));
		add(inlinedFunction(engine, Value(this), L"*--", r, fnPtr(engine, &numPostfixDec<Long>)));
		add(inlinedFunction(engine, Value(this), L"--*", r, fnPtr(engine, &numPrefixDec<Long>)));

		add(inlinedFunction(engine, Value(this, true), L"=", rv, fnPtr(engine, &numAssign<Long>)));
		add(inlinedFunction(engine, Value(this), L"+=", rv, fnPtr(engine, &numInc<Long>)));
		add(inlinedFunction(engine, Value(this), L"-=", rv, fnPtr(engine, &numDec<Long>)));
		add(inlinedFunction(engine, Value(this), L"*=", rv, fnPtr(engine, &numScale<Long>)));
		add(inlinedFunction(engine, Value(this), L"/=", rv, fnPtr(engine, &numIDivScale<Long>)));
		add(inlinedFunction(engine, Value(this), L"%=", rv, fnPtr(engine, &numIModEq<Long>)));

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Long>)));
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Long>)));

		add(inlinedFunction(engine, Value(StormInfo<Int>::type(engine)), L"int", v, fnPtr(engine, &icast)));
		add(inlinedFunction(engine, Value(StormInfo<Nat>::type(engine)), L"nat", v, fnPtr(engine, &icast)));
		add(inlinedFunction(engine, Value(StormInfo<Byte>::type(engine)), L"byte", v, fnPtr(engine, &icast)));
		add(inlinedFunction(engine, Value(StormInfo<Word>::type(engine)), L"word", v, fnPtr(engine, &icast)));
		add(inlinedFunction(engine, Value(StormInfo<Float>::type(engine)), L"float", v, fnPtr(engine, &numToFloat)));

		add(nativeFunction(engine, Value(this), L"hash", v, &longHash));
		add(nativeFunction(engine, Value(this), L"min", vv, address(&numMin<Long>)));
		add(nativeFunction(engine, Value(this), L"max", vv, address(&numMax<Long>)));

		return Type::loadAll();
	}


	Type *createWord(Str *name, Size size, GcType *type) {
		return new (name) WordType(name, type);
	}

	WordType::WordType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sWord, type, null) {}

	Bool WordType::loadAll() {
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

		add(inlinedFunction(engine, Value(this), L"*++", r, fnPtr(engine, &numPostfixInc<Word>)));
		add(inlinedFunction(engine, Value(this), L"++*", r, fnPtr(engine, &numPrefixInc<Word>)));
		add(inlinedFunction(engine, Value(this), L"*--", r, fnPtr(engine, &numPostfixDec<Word>)));
		add(inlinedFunction(engine, Value(this), L"--*", r, fnPtr(engine, &numPrefixDec<Word>)));

		add(inlinedFunction(engine, Value(this, true), L"=", rv, fnPtr(engine, &numAssign<Word>)));
		add(inlinedFunction(engine, Value(this), L"+=", rv, fnPtr(engine, &numInc<Word>)));
		add(inlinedFunction(engine, Value(this), L"-=", rv, fnPtr(engine, &numDec<Word>)));
		add(inlinedFunction(engine, Value(this), L"*=", rv, fnPtr(engine, &numScale<Word>)));
		add(inlinedFunction(engine, Value(this), L"/=", rv, fnPtr(engine, &numUDivScale<Word>)));
		add(inlinedFunction(engine, Value(this), L"%=", rv, fnPtr(engine, &numUModEq<Word>)));

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Word>)));
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Word>)));

		add(inlinedFunction(engine, Value(StormInfo<Int>::type(engine)), L"int", v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Nat>::type(engine)), L"nat", v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Byte>::type(engine)), L"byte", v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Long>::type(engine)), L"long", v, fnPtr(engine, &ucast)));

		add(nativeFunction(engine, Value(this), L"hash", v, &wordHash));
		add(nativeFunction(engine, Value(this), L"min", vv, address(&numMin<Word>)));
		add(nativeFunction(engine, Value(this), L"max", vv, address(&numMax<Word>)));

		return Type::loadAll();
	}

}
