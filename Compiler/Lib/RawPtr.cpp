#include "stdafx.h"
#include "RawPtr.h"
#include "Compiler/Engine.h"
#include "Number.h"
#include "Maybe.h"
#include "Core/Hash.h"
#include "Core/Variant.h"

namespace storm {

	void rawPtrCopy(InlineParams p) {
		p.allocRegs(0, 1);
		*p.state->l << mov(ptrRel(p.regParam(0)), ptrRel(p.regParam(1)));
	}

	void rawPtrInit(InlineParams p) {
		p.allocRegs(0);
		*p.state->l << mov(ptrRel(p.regParam(0)), p.param(1));
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
					*p.state->l << mov(p.result->location(p.state), dest);
			} else {
				// Suggest params[1].
				if (!p.result->suggest(p.state, p.param(1)))
					*p.state->l << mov(p.result->location(p.state), dest);
			}
		}
	}

	void rawPtrEmpty(InlineParams p) {
		using namespace code;

		if (p.result->needed()) {
			*p.state->l << cmp(p.param(0), ptrConst(0));
			*p.state->l << setCond(p.result->location(p.state), ifEqual);
		}
	}

	void rawPtrAny(InlineParams p) {
		using namespace code;

		if (p.result->needed()) {
			*p.state->l << cmp(p.param(0), ptrConst(0));
			*p.state->l << setCond(p.result->location(p.state), ifNotEqual);
		}
	}

	void rawPtrGet(InlineParams p) {
		using namespace code;

		if (p.result->needed()) {
			if (p.result->type().ref) {
				*p.state->l << lea(p.result->location(p.state), p.param(0));
			} else if (!p.result->suggest(p.state, p.param(0))) {
				*p.state->l << mov(p.result->location(p.state), p.param(0));
			}
		}
	}

	Str *CODECALL rawPtrToS(EnginePtr e, RootObject *ptr) {
		StrBuf *buf = new (e) StrBuf();
		*buf << S("0x") << hex(ptr);
		return buf->toS();
	}

	Type *CODECALL rawPtrType(RootObject *ptr) {
		if (ptr)
			return runtime::typeOf(ptr);
		else
			return null;
	}

	Bool CODECALL rawPtrIsValue(const void *ptr) {
		if (!ptr)
			return false;
		const GcType *type = runtime::gcTypeOf(ptr);
		if (type->kind == GcType::tArray || type->kind == GcType::tWeakArray)
			return type->type != null;
		else
			return false;
	}

	template <class T>
	T CODECALL rawPtrRead(const void *ptr, Nat offset) {
		const GcType *t = runtime::gcTypeOf(ptr);
		if (t->kind == GcType::tArray || t->kind == GcType::tWeakArray) {
			offset += OFFSET_OF(GcArray<Byte>, v);
		}

		return *(T *)((byte *)ptr + offset);
	}

	Nat CODECALL rawPtrCount(const void *ptr) {
		if (!ptr)
			return 0;

		const GcType *t = runtime::gcTypeOf(ptr);
		if (t->kind == GcType::tArray) {
			return Nat(((GcArray<byte> *)ptr)->count);
		} else if (t->kind == GcType::tWeakArray) {
			return Nat(((GcWeakArray<void *> *)ptr)->count());
		} else {
			return 1;
		}
	}

	Nat CODECALL rawPtrFilled(const void *ptr) {
		if (!ptr)
			return 0;

		const GcType *t = runtime::gcTypeOf(ptr);
		if (t->kind == GcType::tArray) {
			return Nat(((GcArray<byte> *)ptr)->filled);
		} else if (t->kind == GcType::tWeakArray) {
			return Nat(((GcWeakArray<void *> *)ptr)->splatted());
		} else {
			return 1;
		}
	}

	Nat CODECALL rawPtrSize(const void *ptr) {
		if (!ptr)
			return 0;

		const GcType *t = runtime::gcTypeOf(ptr);
		return t->stride;
	}


	template <Size T()>
	void rawPtrRead(InlineParams p) {
		using namespace code;

		if (!p.result->needed())
			return;

		p.spillParams();

		// Check if it is an array...
		*p.state->l << mov(ptrA, p.param(0));
		*p.state->l << mov(ptrA, ptrRel(ptrA, -Offset::sPtr)); // read type info
		*p.state->l << mov(ptrA, ptrRel(ptrA, Offset())); // read 'kind'.
		*p.state->l << cmp(ptrA, ptrConst(GcType::tArray));
		*p.state->l << cmp(ptrA, ptrConst(GcType::tWeakArray));

		*p.state->l << ucast(ptrA, p.param(1));
		*p.state->l << add(ptrA, p.param(0));

		*p.state->l << mov(p.result->location(p.state), xRel(T(), ptrA, Offset()));
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
		add(nativeFunction(e, Value(StormInfo<Bool>::type(e)), S("readBool"), vo, address(&rawPtrRead<Bool>)));
		add(nativeFunction(e, Value(StormInfo<Byte>::type(e)), S("readByte"), vo, address(&rawPtrRead<Byte>)));
		add(nativeFunction(e, Value(StormInfo<Int>::type(e)), S("readInt"), vo, address(&rawPtrRead<Int>)));
		add(nativeFunction(e, Value(StormInfo<Nat>::type(e)), S("readNat"), vo, address(&rawPtrRead<Nat>)));
		add(nativeFunction(e, Value(StormInfo<Long>::type(e)), S("readLong"), vo, address(&rawPtrRead<Long>)));
		add(nativeFunction(e, Value(StormInfo<Word>::type(e)), S("readWord"), vo, address(&rawPtrRead<Word>)));
		add(nativeFunction(e, Value(StormInfo<Float>::type(e)), S("readFloat"), vo, address(&rawPtrRead<Float>)));
		add(nativeFunction(e, Value(StormInfo<Double>::type(e)), S("readDouble"), vo, address(&rawPtrRead<Double>)));
		add(nativeFunction(e, me, S("readPtr"), vo, address(&rawPtrRead<void *>)));
		add(nativeFunction(e, Value(StormInfo<Nat>::type(e)), S("readCount"), v, address(&rawPtrCount)));
		add(nativeFunction(e, Value(StormInfo<Nat>::type(e)), S("readFilled"), v, address(&rawPtrFilled)));
		add(nativeFunction(e, Value(StormInfo<Nat>::type(e)), S("readSize"), v, address(&rawPtrSize)));

		// Create from pointers:
		Value obj(StormInfo<Object>::type(e));
		Array<Value> *objPar = valList(e, 2, me.asRef(), obj);
		add(inlinedFunction(e, Value(), Type::CTOR, objPar, fnPtr(e, &rawPtrInit)));
		Value tobj(StormInfo<TObject>::type(e));
		Array<Value> *tobjPar = valList(e, 2, me.asRef(), tobj);
		add(inlinedFunction(e, Value(), Type::CTOR, tobjPar, fnPtr(e, &rawPtrInit)));
		Value variant(StormInfo<Variant>::type(e));
		Array<Value> *variantPar = valList(e, 2, me.asRef(), variant.asRef());
		add(inlinedFunction(e, Value(), Type::CTOR, variantPar, fnPtr(e, &rawPtrCopy))); // Same as a copy of a RawPtr.

		// Inspect the data.
		Value type(StormInfo<Type *>::type(e));
		add(nativeFunction(e, wrapMaybe(type), S("type"), v, address(&rawPtrType)));
		add(nativeFunction(e, Value(StormInfo<Bool>::type(e)), S("isValue"), v, address(&rawPtrIsValue)));
		add(inlinedFunction(e, obj, S("asObject"), v, fnPtr(e, &rawPtrGet))->makePure());
		add(inlinedFunction(e, tobj, S("asTObject"), v, fnPtr(e, &rawPtrGet))->makePure());

		return Type::loadAll();
	}

	code::TypeDesc *RawPtrType::createTypeDesc() {
		return engine.ptrDesc();
	}

	void RawPtrType::modifyHandle(Handle *handle) {
		// We're basically a pointer, so use location based hashes!
		handle->locationHash = true;
	}

}
