#include "stdafx.h"
#include "Long.h"
#include "Engine.h"
#include "Code.h"
#include "Function.h"
#include "CodeGen.h"
#include "Number.h"
#include "Shared/Hash.h"

namespace storm {
	using namespace code;

	static void longAssign(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << mov(longRel(ptrA), p.params[1]);
		if (p.result->needed()) {
			if (p.result->type().ref) {
				if (!p.result->suggest(p.state, p.params[0]))
					p.state->to << mov(p.result->location(p.state).var(), ptrA);
			} else {
				if (!p.result->suggest(p.state, p.params[1]))
					p.state->to << mov(p.result->location(p.state).var(), p.params[1]);
			}
		}
	}

	static void createLong(InlinedParams p) {
		p.state->to << mov(ptrC, p.params[0]);
		p.state->to << icast(longRel(ptrC), p.params[1]);
	}

	static void longCopyCtor(InlinedParams p) {
		p.state->to << mov(ptrC, p.params[1]);
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << mov(longRel(ptrA), longRel(ptrC));
	}

	static void longInit(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << mov(longRel(ptrA), longConst(0));
	}

	LongType::LongType() : Type(L"Long", typeValue | typeFinal, Size::sLong, null) {}

	bool LongType::loadAll() {
		vector<Value> r(1, Value(this, true));
		vector<Value> v(1, Value(this));
		vector<Value> vv(2, Value(this));
		vector<Value> rv = valList(2, Value(this, true), Value(this));
		vector<Value> rr(2, Value(this, true));
		Value b(boolType(engine));

		add(steal(inlinedFunction(engine, Value(this), L"+", vv, simpleFn(&numAdd))));
		add(steal(inlinedFunction(engine, Value(this), L"-", vv, simpleFn(&numSub))));
		add(steal(inlinedFunction(engine, Value(this), L"*", vv, simpleFn(&numMul))));
		add(steal(inlinedFunction(engine, Value(this), L"/", vv, simpleFn(&numIdiv))));
		add(steal(inlinedFunction(engine, Value(this), L"%", vv, simpleFn(&numImod))));

		add(steal(inlinedFunction(engine, b, L"==", vv, simpleFn(&numCmp<ifEqual>))));
		add(steal(inlinedFunction(engine, b, L"!=", vv, simpleFn(&numCmp<ifNotEqual>))));
		add(steal(inlinedFunction(engine, b, L"<", vv, simpleFn(&numCmp<ifBelow>))));
		add(steal(inlinedFunction(engine, b, L">", vv, simpleFn(&numCmp<ifAbove>))));
		add(steal(inlinedFunction(engine, b, L"<=", vv, simpleFn(&numCmp<ifBelowEqual>))));
		add(steal(inlinedFunction(engine, b, L">=", vv, simpleFn(&numCmp<ifAboveEqual>))));

		add(steal(inlinedFunction(engine, Value(byteType(engine)), L"byte", v, simpleFn(&icast))));
		add(steal(inlinedFunction(engine, Value(intType(engine)), L"int", v, simpleFn(&icast))));
		add(steal(inlinedFunction(engine, Value(natType(engine)), L"nat", v, simpleFn(&icast))));
		add(steal(inlinedFunction(engine, Value(wordType(engine)), L"word", v, simpleFn(&icast))));

		add(steal(inlinedFunction(engine, Value(this), L"*++", r, simpleFn(&numPostfixInc<Long>))));
		add(steal(inlinedFunction(engine, Value(this), L"++*", r, simpleFn(&numPrefixInc<Long>))));
		add(steal(inlinedFunction(engine, Value(this), L"*--", r, simpleFn(&numPostfixDec<Long>))));
		add(steal(inlinedFunction(engine, Value(this), L"--*", r, simpleFn(&numPrefixDec<Long>))));

		add(steal(inlinedFunction(engine, Value(this, true), L"=", rv, simpleFn(&longAssign))));
		add(steal(inlinedFunction(engine, Value(this), L"+=", rv, simpleFn(&numInc<Long>))));
		add(steal(inlinedFunction(engine, Value(this), L"-=", rv, simpleFn(&numDec<Long>))));
		add(steal(inlinedFunction(engine, Value(this), L"*=", rv, simpleFn(&numScale<Long>))));
		add(steal(inlinedFunction(engine, Value(this), L"/=", rv, simpleFn(&numIDivScale<Long>))));
		add(steal(inlinedFunction(engine, Value(this), L"%=", rv, simpleFn(&numIModEq<Long>))));

		add(steal(inlinedFunction(engine, Value(), Type::CTOR, rr, simpleFn(&longCopyCtor))));
		add(steal(inlinedFunction(engine, Value(), Type::CTOR, r, simpleFn(&longInit))));

		vector<Value> ri = valList(2, Value(this, true), Value(intType(engine)));
		add(stealAutoCast(inlinedFunction(engine, Value(), Type::CTOR, ri, simpleFn(&createLong))));

		add(steal(nativeFunction(engine, Value(natType(engine)), L"hash", v, &longHash)));

		return Type::loadAll();
	}

	Type *longType(Engine &e) {
		Type *t = e.specialBuiltIn(specialLong);
		if (!t) {
			t = CREATE(LongType, e);
			e.setSpecialBuiltIn(specialLong, t);
		}
		return t;
	}


	static void createWord(InlinedParams p) {
		p.state->to << mov(ptrC, p.params[0]);
		p.state->to << ucast(longRel(ptrC), p.params[1]);
	}

	static void initWord(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << mov(longRel(ptrA), longConst(0));
	}

	WordType::WordType() : Type(L"Word", typeValue | typeFinal, Size::sWord, null) {}

	bool WordType::loadAll() {
		vector<Value> r(1, Value(this, true));
		vector<Value> v(1, Value(this));
		vector<Value> vv(2, Value(this));
		vector<Value> rv = valList(2, Value(this, true), Value(this));
		vector<Value> rr(2, Value(this, true));
		Value b(boolType(engine));

		add(steal(inlinedFunction(engine, Value(this), L"+", vv, simpleFn(&numAdd))));
		add(steal(inlinedFunction(engine, Value(this), L"-", vv, simpleFn(&numSub))));
		add(steal(inlinedFunction(engine, Value(this), L"*", vv, simpleFn(&numMul))));
		add(steal(inlinedFunction(engine, Value(this), L"/", vv, simpleFn(&numUdiv))));
		add(steal(inlinedFunction(engine, Value(this), L"%", vv, simpleFn(&numUmod))));

		add(steal(inlinedFunction(engine, b, L"==", vv, simpleFn(&numCmp<ifEqual>))));
		add(steal(inlinedFunction(engine, b, L"!=", vv, simpleFn(&numCmp<ifNotEqual>))));
		add(steal(inlinedFunction(engine, b, L"<", vv, simpleFn(&numCmp<ifBelow>))));
		add(steal(inlinedFunction(engine, b, L">", vv, simpleFn(&numCmp<ifAbove>))));
		add(steal(inlinedFunction(engine, b, L"<=", vv, simpleFn(&numCmp<ifBelowEqual>))));
		add(steal(inlinedFunction(engine, b, L">=", vv, simpleFn(&numCmp<ifAboveEqual>))));

		add(steal(inlinedFunction(engine, Value(byteType(engine)), L"byte", v, simpleFn(&ucast))));
		add(steal(inlinedFunction(engine, Value(intType(engine)), L"int", v, simpleFn(&ucast))));
		add(steal(inlinedFunction(engine, Value(natType(engine)), L"nat", v, simpleFn(&ucast))));
		add(steal(inlinedFunction(engine, Value(longType(engine)), L"long", v, simpleFn(&ucast))));

		add(steal(inlinedFunction(engine, Value(this), L"*++", r, simpleFn(&numPostfixInc<Word>))));
		add(steal(inlinedFunction(engine, Value(this), L"++*", r, simpleFn(&numPrefixInc<Word>))));
		add(steal(inlinedFunction(engine, Value(this), L"*--", r, simpleFn(&numPostfixDec<Word>))));
		add(steal(inlinedFunction(engine, Value(this), L"--*", r, simpleFn(&numPrefixDec<Word>))));

		add(steal(inlinedFunction(engine, Value(), Type::CTOR, rr, simpleFn(&longCopyCtor))));
		add(steal(inlinedFunction(engine, Value(this, true), L"=", rv, simpleFn(&longAssign))));
		add(steal(inlinedFunction(engine, Value(this), L"+=", rv, simpleFn(&numInc<Word>))));
		add(steal(inlinedFunction(engine, Value(this), L"-=", rv, simpleFn(&numDec<Word>))));
		add(steal(inlinedFunction(engine, Value(this), L"*=", rv, simpleFn(&numScale<Word>))));
		add(steal(inlinedFunction(engine, Value(this), L"/=", rv, simpleFn(&numUDivScale<Word>))));
		add(steal(inlinedFunction(engine, Value(this), L"%=", rv, simpleFn(&numUModEq<Word>))));

		vector<Value> rn = valList(2, Value(this, true), Value(natType(engine)));
		vector<Value> rb = valList(2, Value(this, true), Value(byteType(engine)));
		add(stealAutoCast(inlinedFunction(engine, Value(), Type::CTOR, rn, simpleFn(&createWord))));
		add(stealAutoCast(inlinedFunction(engine, Value(), Type::CTOR, rb, simpleFn(&createWord))));
		add(stealAutoCast(inlinedFunction(engine, Value(), Type::CTOR, r, simpleFn(&initWord))));

		add(steal(nativeFunction(engine, Value(natType(engine)), L"hash", v, &wordHash)));

		return Type::loadAll();
	}

	Type *wordType(Engine &e) {
		Type *t = e.specialBuiltIn(specialWord);
		if (!t) {
			t = CREATE(WordType, e);
			e.setSpecialBuiltIn(specialWord, t);
		}
		return t;
	}

	Str *toS(EnginePtr e, Long v) {
		return CREATE(Str, e.v, ::toS(v));
	}

	Str *toS(EnginePtr e, Word v) {
		return CREATE(Str, e.v, ::toS(v));
	}

}
