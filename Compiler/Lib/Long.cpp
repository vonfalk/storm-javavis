#include "stdafx.h"
#include "Long.h"
#include "Core/Array.h"
#include "Core/Hash.h"
#include "Core/Str.h"
#include "Core/Io/Serialization.h"
#include "Function.h"
#include "Number.h"

namespace storm {
	using namespace code;

	Type *createLong(Str *name, Size size, GcType *type) {
		return new (name) LongType(name, type);
	}

	// Note: Works for both float and double.
	static void longToFloat(InlineParams p) {
		if (!p.result->needed())
			return;

		*p.state->l << fild(p.param(0));
		*p.state->l << fstp(p.result->location(p.state).v);
	}

	static void castLong(InlineParams p) {
		p.allocRegs(0);
		*p.state->l << icast(longRel(p.regParam(0), Offset()), p.param(1));
	}

	static Long longRead(ObjIStream *from) {
		return from->readLong();
	}

	static void longWrite(Long v, ObjOStream *to) {
		to->writeLong(v);
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

		add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &numAdd))->makePure());
		add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &numSub))->makePure());
		add(inlinedFunction(engine, Value(this), S("*"), vv, fnPtr(engine, &numMul))->makePure());
		add(inlinedFunction(engine, Value(this), S("/"), vv, fnPtr(engine, &numIDiv))->makePure());
		add(inlinedFunction(engine, Value(this), S("%"), vv, fnPtr(engine, &numIMod))->makePure());
		add(inlinedFunction(engine, Value(this), S("-"), v, fnPtr(engine, &numINeg))->makePure());

		add(inlinedFunction(engine, b, S("=="), vv, fnPtr(engine, &numCmp<ifEqual>))->makePure());
		add(inlinedFunction(engine, b, S("!="), vv, fnPtr(engine, &numCmp<ifNotEqual>))->makePure());
		add(inlinedFunction(engine, b, S("<="), vv, fnPtr(engine, &numCmp<ifLessEqual>))->makePure());
		add(inlinedFunction(engine, b, S(">="), vv, fnPtr(engine, &numCmp<ifGreaterEqual>))->makePure());
		add(inlinedFunction(engine, b, S("<"), vv, fnPtr(engine, &numCmp<ifLess>))->makePure());
		add(inlinedFunction(engine, b, S(">"), vv, fnPtr(engine, &numCmp<ifGreater>))->makePure());

		add(inlinedFunction(engine, Value(this), S("*++"), r, fnPtr(engine, &numPostfixInc<Long>))->makePure());
		add(inlinedFunction(engine, Value(this), S("++*"), r, fnPtr(engine, &numPrefixInc<Long>))->makePure());
		add(inlinedFunction(engine, Value(this), S("*--"), r, fnPtr(engine, &numPostfixDec<Long>))->makePure());
		add(inlinedFunction(engine, Value(this), S("--*"), r, fnPtr(engine, &numPrefixDec<Long>))->makePure());

		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &numAssign<Long>))->makePure());
		add(inlinedFunction(engine, Value(this), S("+="), rv, fnPtr(engine, &numInc<Long>))->makePure());
		add(inlinedFunction(engine, Value(this), S("-="), rv, fnPtr(engine, &numDec<Long>))->makePure());
		add(inlinedFunction(engine, Value(this), S("*="), rv, fnPtr(engine, &numScale<Long>))->makePure());
		add(inlinedFunction(engine, Value(this), S("/="), rv, fnPtr(engine, &numIDivScale<Long>))->makePure());
		add(inlinedFunction(engine, Value(this), S("%="), rv, fnPtr(engine, &numIModEq<Long>))->makePure());

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Long>))->makePure());
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Long>))->makePure());

		Array<Value> *ri = valList(engine, 2, Value(this, true), Value(StormInfo<Int>::type(engine)));
		add(inlinedFunction(engine, Value(), Type::CTOR, ri, fnPtr(engine, &castLong))->makeAutoCast()->makePure());

		add(inlinedFunction(engine, Value(StormInfo<Int>::type(engine)), S("int"), v, fnPtr(engine, &icast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Nat>::type(engine)), S("nat"), v, fnPtr(engine, &icast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Byte>::type(engine)), S("byte"), v, fnPtr(engine, &icast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Word>::type(engine)), S("word"), v, fnPtr(engine, &icast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Float>::type(engine)), S("float"), v, fnPtr(engine, &longToFloat))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Double>::type(engine)), S("double"), v, fnPtr(engine, &longToFloat))->makePure());

		Value n(StormInfo<Nat>::type(engine));
		add(nativeFunction(engine, n, S("hash"), v, address(&longHash))->makePure());
		add(inlinedFunction(engine, Value(this), S("min"), vv, fnPtr(engine, &numMin<Long>))->makePure());
		add(inlinedFunction(engine, Value(this), S("max"), vv, fnPtr(engine, &numMax<Long>))->makePure());
		add(inlinedFunction(engine, Value(this), S("delta"), vv, fnPtr(engine, &numDelta<Long>))->makePure());

		Array<Value> *is = new (this) Array<Value>(1, Value(StormInfo<ObjIStream>::type(engine)));
		add(nativeFunction(engine, Value(this), S("read"), is, address(&longRead)));

		Array<Value> *os = new (this) Array<Value>(2, Value(this, false));
		os->at(1) = Value(StormInfo<ObjOStream>::type(engine));
		add(nativeFunction(engine, Value(), S("write"), os, address(&longWrite)));

		return Type::loadAll();
	}


	Type *createWord(Str *name, Size size, GcType *type) {
		return new (name) WordType(name, type);
	}

	static void castWord(InlineParams p) {
		p.allocRegs(0);
		*p.state->l << ucast(longRel(p.regParam(0), Offset()), p.param(1));
	}

	static Word wordRead(ObjIStream *from) {
		return from->readWord();
	}

	static void wordWrite(Word v, ObjOStream *to) {
		to->writeWord(v);
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

		add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &numAdd))->makePure());
		add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &numSub))->makePure());
		add(inlinedFunction(engine, Value(this), S("*"), vv, fnPtr(engine, &numMul))->makePure());
		add(inlinedFunction(engine, Value(this), S("/"), vv, fnPtr(engine, &numUDiv))->makePure());
		add(inlinedFunction(engine, Value(this), S("%"), vv, fnPtr(engine, &numUMod))->makePure());

		add(inlinedFunction(engine, b, S("=="), vv, fnPtr(engine, &numCmp<ifEqual>))->makePure());
		add(inlinedFunction(engine, b, S("!="), vv, fnPtr(engine, &numCmp<ifNotEqual>))->makePure());
		add(inlinedFunction(engine, b, S("<="), vv, fnPtr(engine, &numCmp<ifBelowEqual>))->makePure());
		add(inlinedFunction(engine, b, S(">="), vv, fnPtr(engine, &numCmp<ifAboveEqual>))->makePure());
		add(inlinedFunction(engine, b, S("<"), vv, fnPtr(engine, &numCmp<ifBelow>))->makePure());
		add(inlinedFunction(engine, b, S(">"), vv, fnPtr(engine, &numCmp<ifAbove>))->makePure());

		add(inlinedFunction(engine, Value(this), S("*++"), r, fnPtr(engine, &numPostfixInc<Word>))->makePure());
		add(inlinedFunction(engine, Value(this), S("++*"), r, fnPtr(engine, &numPrefixInc<Word>))->makePure());
		add(inlinedFunction(engine, Value(this), S("*--"), r, fnPtr(engine, &numPostfixDec<Word>))->makePure());
		add(inlinedFunction(engine, Value(this), S("--*"), r, fnPtr(engine, &numPrefixDec<Word>))->makePure());

		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &numAssign<Word>))->makePure());
		add(inlinedFunction(engine, Value(this), S("+="), rv, fnPtr(engine, &numInc<Word>))->makePure());
		add(inlinedFunction(engine, Value(this), S("-="), rv, fnPtr(engine, &numDec<Word>))->makePure());
		add(inlinedFunction(engine, Value(this), S("*="), rv, fnPtr(engine, &numScale<Word>))->makePure());
		add(inlinedFunction(engine, Value(this), S("/="), rv, fnPtr(engine, &numUDivScale<Word>))->makePure());
		add(inlinedFunction(engine, Value(this), S("%="), rv, fnPtr(engine, &numUModEq<Word>))->makePure());

		// Bitwise operators.
		add(inlinedFunction(engine, Value(this), S("&"), vv, fnPtr(engine, &numAnd))->makePure());
		add(inlinedFunction(engine, Value(this), S("|"), vv, fnPtr(engine, &numOr))->makePure());
		add(inlinedFunction(engine, Value(this), S("^"), vv, fnPtr(engine, &numXor))->makePure());
		add(inlinedFunction(engine, Value(this), S("~"), v, fnPtr(engine, &numNot))->makePure());

		add(inlinedFunction(engine, Value(this), S("&="), rv, fnPtr(engine, &numAndEq<Word>))->makePure());
		add(inlinedFunction(engine, Value(this), S("|="), rv, fnPtr(engine, &numOrEq<Word>))->makePure());
		add(inlinedFunction(engine, Value(this), S("^="), rv, fnPtr(engine, &numXorEq<Word>))->makePure());

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Word>))->makePure());
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Word>))->makePure());

		Array<Value> *rb = valList(engine, 2, Value(this, true), Value(StormInfo<Byte>::type(engine)));
		add(inlinedFunction(engine, Value(), Type::CTOR, rb, fnPtr(engine, &castWord))->makeAutoCast()->makePure());
		Array<Value> *ri = valList(engine, 2, Value(this, true), Value(StormInfo<Nat>::type(engine)));
		add(inlinedFunction(engine, Value(), Type::CTOR, ri, fnPtr(engine, &castWord))->makeAutoCast()->makePure());

		add(inlinedFunction(engine, Value(StormInfo<Int>::type(engine)), S("int"), v, fnPtr(engine, &ucast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Nat>::type(engine)), S("nat"), v, fnPtr(engine, &ucast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Byte>::type(engine)), S("byte"), v, fnPtr(engine, &ucast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Long>::type(engine)), S("long"), v, fnPtr(engine, &ucast))->makePure());

		Value n(StormInfo<Nat>::type(engine));
		add(nativeFunction(engine, n, S("hash"), v, address(&wordHash))->makePure());
		add(inlinedFunction(engine, Value(this), S("min"), vv, fnPtr(engine, &numMin<Word>))->makePure());
		add(inlinedFunction(engine, Value(this), S("max"), vv, fnPtr(engine, &numMax<Word>))->makePure());
		add(inlinedFunction(engine, Value(this), S("delta"), vv, fnPtr(engine, &numDelta<Word>))->makePure());

		Array<Value> *is = new (this) Array<Value>(1, Value(StormInfo<ObjIStream>::type(engine)));
		add(nativeFunction(engine, Value(this), S("read"), is, address(&wordRead)));

		Array<Value> *os = new (this) Array<Value>(2, Value(this, false));
		os->at(1) = Value(StormInfo<ObjOStream>::type(engine));
		add(nativeFunction(engine, Value(), S("write"), os, address(&wordWrite)));

		return Type::loadAll();
	}

}
