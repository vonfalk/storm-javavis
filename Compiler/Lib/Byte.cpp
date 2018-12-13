#include "stdafx.h"
#include "Byte.h"
#include "Core/Array.h"
#include "Core/Hash.h"
#include "Core/Str.h"
#include "Core/Io/Serialization.h"
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

	Byte byteRead(IStream *from) {
		return from->readByte();
	}

	void byteWrite(Byte b, OStream *to) {
		to->writeByte(b);
	}

	void byteWriteS(Byte b, ObjOStream *to) {
		to->startCustom(byteId);
		to->to->writeByte(b);
		to->end();
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

		add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &numAdd))->makePure());
		add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &numSub))->makePure());
		add(inlinedFunction(engine, Value(this), S("*"), vv, fnPtr(engine, &numMul))->makePure());
		add(inlinedFunction(engine, Value(this), S("/"), vv, fnPtr(engine, &numIDiv))->makePure());
		add(inlinedFunction(engine, Value(this), S("%"), vv, fnPtr(engine, &numIMod))->makePure());

		add(inlinedFunction(engine, b, S("=="), vv, fnPtr(engine, &numCmp<ifEqual>))->makePure());
		add(inlinedFunction(engine, b, S("!="), vv, fnPtr(engine, &numCmp<ifNotEqual>))->makePure());
		add(inlinedFunction(engine, b, S("<="), vv, fnPtr(engine, &numCmp<ifLessEqual>))->makePure());
		add(inlinedFunction(engine, b, S(">="), vv, fnPtr(engine, &numCmp<ifGreaterEqual>))->makePure());
		add(inlinedFunction(engine, b, S("<"), vv, fnPtr(engine, &numCmp<ifLess>))->makePure());
		add(inlinedFunction(engine, b, S(">"), vv, fnPtr(engine, &numCmp<ifGreater>))->makePure());

		add(inlinedFunction(engine, Value(this), S("*++"), r, fnPtr(engine, &numPostfixInc<Byte>))->makePure());
		add(inlinedFunction(engine, Value(this), S("++*"), r, fnPtr(engine, &numPrefixInc<Byte>))->makePure());
		add(inlinedFunction(engine, Value(this), S("*--"), r, fnPtr(engine, &numPostfixDec<Byte>))->makePure());
		add(inlinedFunction(engine, Value(this), S("--*"), r, fnPtr(engine, &numPrefixDec<Byte>))->makePure());

		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &numAssign<Byte>))->makePure());
		add(inlinedFunction(engine, Value(this), S("+="), rv, fnPtr(engine, &numInc<Byte>))->makePure());
		add(inlinedFunction(engine, Value(this), S("-="), rv, fnPtr(engine, &numDec<Byte>))->makePure());
		add(inlinedFunction(engine, Value(this), S("*="), rv, fnPtr(engine, &numScale<Byte>))->makePure());
		add(inlinedFunction(engine, Value(this), S("/="), rv, fnPtr(engine, &numIDivScale<Byte>))->makePure());
		add(inlinedFunction(engine, Value(this), S("%="), rv, fnPtr(engine, &numIModEq<Byte>))->makePure());

		// Bitwise operators.
		add(inlinedFunction(engine, Value(this), S("&"), vv, fnPtr(engine, &numAnd))->makePure());
		add(inlinedFunction(engine, Value(this), S("|"), vv, fnPtr(engine, &numOr))->makePure());
		add(inlinedFunction(engine, Value(this), S("^"), vv, fnPtr(engine, &numXor))->makePure());
		add(inlinedFunction(engine, Value(this), S("~"), v, fnPtr(engine, &numNot))->makePure());

		add(inlinedFunction(engine, Value(this), S("&="), rv, fnPtr(engine, &numAndEq<Byte>))->makePure());
		add(inlinedFunction(engine, Value(this), S("|="), rv, fnPtr(engine, &numOrEq<Byte>))->makePure());
		add(inlinedFunction(engine, Value(this), S("^="), rv, fnPtr(engine, &numXorEq<Byte>))->makePure());


		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Byte>))->makePure());
		add(inlinedFunction(engine, Value(), Type::CTOR, r, fnPtr(engine, &numInit<Byte>))->makePure());

		add(inlinedFunction(engine, Value(StormInfo<Int>::type(engine)), S("int"), v, fnPtr(engine, &ucast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Nat>::type(engine)), S("nat"), v, fnPtr(engine, &ucast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Long>::type(engine)), S("long"), v, fnPtr(engine, &ucast))->makePure());
		add(inlinedFunction(engine, Value(StormInfo<Word>::type(engine)), S("word"), v, fnPtr(engine, &ucast))->makePure());
		add(nativeFunction(engine, Value(StormInfo<Float>::type(engine)), S("float"), v, address(&byteToFloat))->makePure());

		Value n(StormInfo<Nat>::type(engine));
		add(nativeFunction(engine, n, S("hash"), v, address(&byteHash))->makePure());
		add(inlinedFunction(engine, Value(this), S("min"), vv, fnPtr(engine, &numMin<Byte>))->makePure());
		add(inlinedFunction(engine, Value(this), S("max"), vv, fnPtr(engine, &numMax<Byte>))->makePure());
		add(inlinedFunction(engine, Value(this), S("delta"), vv, fnPtr(engine, &numDelta<Byte>))->makePure());

		Array<Value> *is = new (this) Array<Value>(1, Value(StormInfo<IStream>::type(engine)));
		add(nativeFunction(engine, Value(this), S("read"), is, address(&byteRead)));

		Array<Value> *os = new (this) Array<Value>(2, Value(this, false));
		os->at(1) = Value(StormInfo<OStream>::type(engine));
		add(nativeFunction(engine, Value(), S("write"), os, address(&byteWrite)));

		os = new (this) Array<Value>(2, Value(this, false));
		os->at(1) = Value(StormInfo<ObjOStream>::type(engine));
		add(nativeFunction(engine, Value(), S("write"), os, address(&byteWriteS)));

		return Type::loadAll();
	}

}
