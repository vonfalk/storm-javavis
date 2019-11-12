#include "stdafx.h"
#include "RawPtr.h"
#include "Compiler/Engine.h"
#include "Number.h"
#include "Core/Hash.h"

namespace storm {

	void rawPtrCopy(InlineParams p) {
		p.allocRegs(0, 1);
		*p.state->l << mov(ptrRel(p.regParam(0), Offset()), ptrRel(p.regParam(1), Offset()));
	}

	void rawPtrInit(InlineParams p) {
		p.allocRegs(0);
		*p.state->l << mov(ptrRel(p.regParam(0), Offset()), p.param(1));
	}

	void rawPtrAssign(InlineParams p) {
		using namespace code;

		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << mov(ptrRel(dest, Offset()), p.param(1));
		if (p.result->needed()) {
			if (p.result->type().ref) {
				// Suggest params[0].
				if (!p.result->suggest(p.state, p.originalParam(0)))
					*p.state->l << mov(p.result->location(p.state).v, dest);
			} else {
				// Suggest params[1].
				if (!p.result->suggest(p.state, p.param(1)))
					*p.state->l << mov(p.result->location(p.state).v, dest);
			}
		}
	}

	void rawPtrEmpty(InlineParams p) {
		using namespace code;

		*p.state->l << cmp(p.param(0), ptrConst(0));
		*p.state->l << setCond(p.result->location(p.state).v, ifEqual);
	}

	void rawPtrAny(InlineParams p) {
		using namespace code;

		*p.state->l << cmp(p.param(0), ptrConst(0));
		*p.state->l << setCond(p.result->location(p.state).v, ifNotEqual);
	}

	Str *CODECALL rawPtrToS(EnginePtr e, RootObject *ptr) {
		StrBuf *buf = new (e) StrBuf();
		*buf << S("0x") << hex(ptr);
		return buf->toS();
	}

	Type *CODECALL rawPtrType(RootObject *ptr) {
		return runtime::typeOf(ptr);
	}

	template <Size T()>
	void rawPtrRead(InlineParams p) {
		using namespace code;

		p.spillParams();
		*p.state->l << ucast(ptrA, p.param(1));
		*p.state->l << add(ptrA, p.param(0));

		*p.state->l << mov(p.result->location(p.state).v, xRel(T(), ptrA, Offset()));
	}

	RawPtrType::RawPtrType(Engine &e) :
		Type(new (e) Str(S("RawPtr")), new (e) Array<Value>(), typeValue | typeFinal, Size::sPtr) {}

	Bool RawPtrType::loadAll() {
		Engine &e = engine;

		Value me(this, false);
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this, false);
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));

		Value b(StormInfo<Bool>::type(e));
		Value n(StormInfo<Nat>::type(e));
		Value str(StormInfo<Str *>::type(e));

		add(inlinedFunction(e, Value(), Type::CTOR, rr, fnPtr(e, &rawPtrCopy))->makePure());
		add(inlinedFunction(e, Value(this, true), S("="), rv, fnPtr(e, &rawPtrAssign)));

		add(inlinedFunction(e, b, S("=="), vv, fnPtr(e, &numCmp<code::ifEqual>))->makePure());
		add(inlinedFunction(e, b, S("!="), vv, fnPtr(e, &numCmp<code::ifNotEqual>))->makePure());
		add(nativeFunction(e, n, S("hash"), v, address(&ptrHash)));
		add(nativeEngineFunction(e, str, S("toS"), v, address(&rawPtrToS)));
		add(inlinedFunction(e, b, S("empty"), v, fnPtr(e, &rawPtrEmpty))->makePure());
		add(inlinedFunction(e, b, S("any"), v, fnPtr(e, &rawPtrAny))->makePure());

		// Access.
		Array<Value> *vo = new (this) Array<Value>(2, Value(this, false));
		vo->at(1) = Value(StormInfo<Nat>::type(e));
		add(inlinedFunction(e, Value(StormInfo<Bool>::type(e)), S("readBool"), vo, fnPtr(e, &rawPtrRead<code::sByte>)));
		add(inlinedFunction(e, Value(StormInfo<Byte>::type(e)), S("readByte"), vo, fnPtr(e, &rawPtrRead<code::sByte>)));
		add(inlinedFunction(e, Value(StormInfo<Int>::type(e)), S("readInt"), vo, fnPtr(e, &rawPtrRead<code::sInt>)));
		add(inlinedFunction(e, Value(StormInfo<Nat>::type(e)), S("readNat"), vo, fnPtr(e, &rawPtrRead<code::sNat>)));
		add(inlinedFunction(e, Value(StormInfo<Long>::type(e)), S("readLong"), vo, fnPtr(e, &rawPtrRead<code::sLong>)));
		add(inlinedFunction(e, Value(StormInfo<Word>::type(e)), S("readWord"), vo, fnPtr(e, &rawPtrRead<code::sWord>)));
		add(inlinedFunction(e, Value(StormInfo<Float>::type(e)), S("readFloat"), vo, fnPtr(e, &rawPtrRead<code::sFloat>)));
		add(inlinedFunction(e, Value(StormInfo<Double>::type(e)), S("readDouble"), vo, fnPtr(e, &rawPtrRead<code::sDouble>)));
		add(inlinedFunction(e, me, S("readPtr"), vo, fnPtr(e, &rawPtrRead<code::sPtr>)));

		// Create from pointers:
		Array<Value> *obj = valList(e, 2, me, Value(StormInfo<Object>::type(e)));
		add(inlinedFunction(e, Value(), Type::CTOR, obj, fnPtr(e, &rawPtrInit)));
		Array<Value> *tobj = valList(e, 2, me, Value(StormInfo<TObject>::type(e)));
		add(inlinedFunction(e, Value(), Type::CTOR, tobj, fnPtr(e, &rawPtrInit)));

		// Inspect the data.
		Value type(StormInfo<Type *>::type(e));
		add(nativeFunction(e, type, S("type"), v, address(&rawPtrType)));

		return Type::loadAll();
	}

	code::TypeDesc *RawPtrType::createTypeDesc() {
		return engine.ptrDesc();
	}

	void RawPtrType::modifyHandle(Handle *handle) {
		// We're basically a pointer, so use location based hashes!
		handle->locationHash = false;
	}

}
