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

	static float CODECALL longToFloat(Long l) {
		return float(l);
	}

	static void castLong(InlineParams p) {
		*p.state->l << mov(ptrA, p.params->at(0));
		*p.state->l << icast(longRel(ptrA, Offset()), p.params->at(1));
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

		add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &numAdd)));
		add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &numSub)));
		add(inlinedFunction(engine, Value(this), S("*"), vv, fnPtr(engine, &numMul)));
		add(inlinedFunction(engine, Value(this), S("/"), vv, fnPtr(engine, &numIDiv)));
		add(inlinedFunction(engine, Value(this), S("%"), vv, fnPtr(engine, &numIMod)));

		add(inlinedFunction(engine, b, S("=="), vv, fnPtr(engine, &numCmp<ifEqual>)));
		add(inlinedFunction(engine, b, S("!="), vv, fnPtr(engine, &numCmp<ifNotEqual>)));
		add(inlinedFunction(engine, b, S("<="), vv, fnPtr(engine, &numCmp<ifLessEqual>)));
		add(inlinedFunction(engine, b, S(">="), vv, fnPtr(engine, &numCmp<ifGreaterEqual>)));
		add(inlinedFunction(engine, b, S("<"), vv, fnPtr(engine, &numCmp<ifLess>)));
		add(inlinedFunction(engine, b, S(">"), vv, fnPtr(engine, &numCmp<ifGreater>)));

		add(inlinedFunction(engine, Value(this), S("*++"), r, fnPtr(engine, &numPostfixInc<Long>)));
		add(inlinedFunction(engine, Value(this), S("++*"), r, fnPtr(engine, &numPrefixInc<Long>)));
		add(inlinedFunction(engine, Value(this), S("*--"), r, fnPtr(engine, &numPostfixDec<Long>)));
		add(inlinedFunction(engine, Value(this), S("--*"), r, fnPtr(engine, &numPrefixDec<Long>)));

		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &numAssign<Long>)));
		add(inlinedFunction(engine, Value(this), S("+="), rv, fnPtr(engine, &numInc<Long>)));
		add(inlinedFunction(engine, Value(this), S("-="), rv, fnPtr(engine, &numDec<Long>)));
		add(inlinedFunction(engine, Value(this), S("*="), rv, fnPtr(engine, &numScale<Long>)));
		add(inlinedFunction(engine, Value(this), S("/="), rv, fnPtr(engine, &numIDivScale<Long>)));
		add(inlinedFunction(engine, Value(this), S("%="), rv, fnPtr(engine, &numIModEq<Long>)));

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Long>)));
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Long>)));

		Array<Value> *ri = valList(engine, 2, Value(this, true), Value(StormInfo<Int>::type(engine)));
		add(cast(inlinedFunction(engine, Value(), Type::CTOR, ri, fnPtr(engine, &castLong))));

		add(inlinedFunction(engine, Value(StormInfo<Int>::type(engine)), S("int"), v, fnPtr(engine, &icast)));
		add(inlinedFunction(engine, Value(StormInfo<Nat>::type(engine)), S("nat"), v, fnPtr(engine, &icast)));
		add(inlinedFunction(engine, Value(StormInfo<Byte>::type(engine)), S("byte"), v, fnPtr(engine, &icast)));
		add(inlinedFunction(engine, Value(StormInfo<Word>::type(engine)), S("word"), v, fnPtr(engine, &icast)));
		add(nativeFunction(engine, Value(StormInfo<Float>::type(engine)), S("float"), v, address(&longToFloat)));

		Value n(StormInfo<Nat>::type(engine));
		add(nativeFunction(engine, n, S("hash"), v, address(&longHash)));
		add(inlinedFunction(engine, Value(this), S("min"), vv, fnPtr(engine, &numMin<Long>)));
		add(inlinedFunction(engine, Value(this), S("max"), vv, fnPtr(engine, &numMax<Long>)));
		add(inlinedFunction(engine, Value(this), S("delta"), vv, fnPtr(engine, &numDelta<Long>)));

		return Type::loadAll();
	}


	Type *createWord(Str *name, Size size, GcType *type) {
		return new (name) WordType(name, type);
	}

	static void castWord(InlineParams p) {
		*p.state->l << mov(ptrA, p.params->at(0));
		*p.state->l << ucast(longRel(ptrA, Offset()), p.params->at(1));
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

		add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &numAdd)));
		add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &numSub)));
		add(inlinedFunction(engine, Value(this), S("*"), vv, fnPtr(engine, &numMul)));
		add(inlinedFunction(engine, Value(this), S("/"), vv, fnPtr(engine, &numUDiv)));
		add(inlinedFunction(engine, Value(this), S("%"), vv, fnPtr(engine, &numUMod)));

		add(inlinedFunction(engine, b, S("=="), vv, fnPtr(engine, &numCmp<ifEqual>)));
		add(inlinedFunction(engine, b, S("!="), vv, fnPtr(engine, &numCmp<ifNotEqual>)));
		add(inlinedFunction(engine, b, S("<="), vv, fnPtr(engine, &numCmp<ifBelowEqual>)));
		add(inlinedFunction(engine, b, S(">="), vv, fnPtr(engine, &numCmp<ifAboveEqual>)));
		add(inlinedFunction(engine, b, S("<"), vv, fnPtr(engine, &numCmp<ifBelow>)));
		add(inlinedFunction(engine, b, S(">"), vv, fnPtr(engine, &numCmp<ifAbove>)));

		add(inlinedFunction(engine, Value(this), S("*++"), r, fnPtr(engine, &numPostfixInc<Word>)));
		add(inlinedFunction(engine, Value(this), S("++*"), r, fnPtr(engine, &numPrefixInc<Word>)));
		add(inlinedFunction(engine, Value(this), S("*--"), r, fnPtr(engine, &numPostfixDec<Word>)));
		add(inlinedFunction(engine, Value(this), S("--*"), r, fnPtr(engine, &numPrefixDec<Word>)));

		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &numAssign<Word>)));
		add(inlinedFunction(engine, Value(this), S("+="), rv, fnPtr(engine, &numInc<Word>)));
		add(inlinedFunction(engine, Value(this), S("-="), rv, fnPtr(engine, &numDec<Word>)));
		add(inlinedFunction(engine, Value(this), S("*="), rv, fnPtr(engine, &numScale<Word>)));
		add(inlinedFunction(engine, Value(this), S("/="), rv, fnPtr(engine, &numUDivScale<Word>)));
		add(inlinedFunction(engine, Value(this), S("%="), rv, fnPtr(engine, &numUModEq<Word>)));

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Word>)));
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Word>)));

		Array<Value> *rb = valList(engine, 2, Value(this, true), Value(StormInfo<Byte>::type(engine)));
		add(cast(inlinedFunction(engine, Value(), Type::CTOR, rb, fnPtr(engine, &castWord))));
		Array<Value> *ri = valList(engine, 2, Value(this, true), Value(StormInfo<Nat>::type(engine)));
		add(cast(inlinedFunction(engine, Value(), Type::CTOR, ri, fnPtr(engine, &castWord))));

		add(inlinedFunction(engine, Value(StormInfo<Int>::type(engine)), S("int"), v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Nat>::type(engine)), S("nat"), v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Byte>::type(engine)), S("byte"), v, fnPtr(engine, &ucast)));
		add(inlinedFunction(engine, Value(StormInfo<Long>::type(engine)), S("long"), v, fnPtr(engine, &ucast)));

		Value n(StormInfo<Nat>::type(engine));
		add(nativeFunction(engine, n, S("hash"), v, address(&wordHash)));
		add(inlinedFunction(engine, Value(this), S("min"), vv, fnPtr(engine, &numMin<Word>)));
		add(inlinedFunction(engine, Value(this), S("max"), vv, fnPtr(engine, &numMax<Word>)));
		add(inlinedFunction(engine, Value(this), S("delta"), vv, fnPtr(engine, &numDelta<Word>)));

		return Type::loadAll();
	}

}
