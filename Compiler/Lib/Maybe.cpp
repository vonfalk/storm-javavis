#include "stdafx.h"
#include "Maybe.h"
#include "Engine.h"
#include "Package.h"
#include "Exception.h"
#include "Core/CloneEnv.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace storm {

	static Bool isValue(Value v) {
		return v.isValue() || v.isBuiltIn();
	}

	static Bool isValue(Type *t) {
		return isValue(Value(t));
	}

	Type *createMaybe(Str *name, ValueArray *val) {
		if (val->count() != 1)
			return null;

		Value p = val->at(0);
		if (p.ref)
			return null;
		if (isMaybe(p))
			return null;

		if (isValue(p))
			return new (name) MaybeValueType(name, p.type);
		else
			return new (name) MaybeClassType(name, p.type);
	}


	Bool isMaybe(Value v) {
		return as<MaybeType>(v.type) != null;
	}

	Value unwrapMaybe(Value v) {
		if (MaybeType *t = as<MaybeType>(v.type))
			return t->param();
		else
			return v;
	}

	Value wrapMaybe(Value v) {
		if (isMaybe(v))
			return v;
		if (v == Value())
			return v;
		Engine &e = v.type->engine;
		TemplateList *l = e.cppTemplate(MaybeId);
		NameSet *to = l->addTo();
		assert(to, L"Too early to use 'wrapMaybe'.");
		Type *found = as<Type>(to->find(S("Maybe"), v, Scope()));
		if (!found)
			throw InternalError(L"Can not find the maybe type!");
		return Value(found);
	}

	static GcType *allocType(Engine &e) {
		GcType *t = e.gc.allocType(GcType::tArray, null, sizeof(void *), 1);
		t->offset[0] = 0;
		return t;
	}

	/**
	 * Common base class.
	 */

	MaybeType::MaybeType(Str *name, Type *param, TypeFlags flags, Size size, GcType *gcType)
		: Type(name, new (name) Array<Value>(1, Value(param)), flags, size, gcType, null), contained(param) {}


	Value MaybeType::param() const {
		return Value(contained);
	}

	/**
	 * For classes or actors.
	 */

	MaybeClassType::MaybeClassType(Str *name, Type *param)
		: MaybeType(name, param, typeClass | typeRawPtr | typeFinal, Size::sPtr, allocType(name->engine())) {}

	static void initMaybeClass(InlineParams p) {
		using namespace code;
		p.allocRegs(0);
		*p.state->l << mov(ptrRel(p.regParam(0), Offset()), ptrConst(Offset()));
	}

	static void copyMaybeClass(InlineParams p) {
		using namespace code;
		p.allocRegs(0);
		Reg dest = p.regParam(0);

		*p.state->l << mov(ptrRel(dest, Offset()), p.param(1));

		// See if we should return something (necessary since we can be used as an assignment).
		if (p.result->needed()) {
			if (!p.result->suggest(p.state, p.originalParam(0))) {
				*p.state->l << mov(p.result->location(p.state).v, dest);
			}
		}
	}

	static void emptyMaybeClass(InlineParams p) {
		using namespace code;
		if (!p.result->needed())
			return;

		*p.state->l << cmp(p.param(0), ptrConst(Offset()));
		*p.state->l << setCond(p.result->location(p.state).v, ifEqual);
	}

	static void anyMaybeClass(InlineParams p) {
		using namespace code;
		if (!p.result->needed())
			return;

		*p.state->l << cmp(p.param(0), ptrConst(Offset()));
		*p.state->l << setCond(p.result->location(p.state).v, ifNotEqual);
	}

	static Str *maybeToSClass(EnginePtr e, RootObject *o) {
		if (!o)
			return new (e.v) Str(L"null");
		else
			return o->toS();
	}

	static void maybeToSBufClass(RootObject *o, StrBuf *buf) {
		if (o)
			o->toS(buf);
		else
			*buf << L"null";
	}

	static void maybeCloneClass(Object *o, CloneEnv *env) {
		if (o)
			o->deepCopy(env);
	}

	code::TypeDesc *MaybeClassType::createTypeDesc() {
		return engine.ptrDesc();
	}

	Bool MaybeClassType::loadAll() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value b = Value(StormInfo<Bool>::type(e));

		Array<Value> *v = new (e) Array<Value>(1, t);
		Array<Value> *r = new (e) Array<Value>(1, t.asRef(true));
		Array<Value> *rv = new (e) Array<Value>(2, t);
		rv->at(0) = t.asRef(true);
		Array<Value> *rp = new (e) Array<Value>(2, t.asRef(true));
		rp->at(1) = param();

		add(inlinedFunction(e, Value(), CTOR, r, fnPtr(e, &initMaybeClass)));
		add(inlinedFunction(e, Value(), CTOR, rv, fnPtr(e, &copyMaybeClass)));
		add(inlinedFunction(e, t.asRef(true), S("="), rv, fnPtr(e, &copyMaybeClass)));
		add(inlinedFunction(e, b, S("empty"), v, fnPtr(e, &emptyMaybeClass)));
		add(inlinedFunction(e, b, S("any"), v, fnPtr(e, &anyMaybeClass)));

		// Cast constructor.
		Function *cast = inlinedFunction(e, Value(), CTOR, rp, fnPtr(e, &copyMaybeClass));
		cast->flags |= namedAutoCast;
		add(cast);

		add(nativeEngineFunction(e, Value(Str::stormType(e)), S("toS"), v, address(&maybeToSClass)));

		Array<Value> *strBuf = new (e) Array<Value>(2, t);
		strBuf->at(1) = Value(StrBuf::stormType(e));
		add(nativeFunction(e, Value(), S("toS"), strBuf, address(&maybeToSBufClass)));

		if (!contained->isA(TObject::stormType(e))) {
			Array<Value> *clone = new (e) Array<Value>(2, t);
			clone->at(1) = Value(CloneEnv::stormType(e));
			add(nativeFunction(e, Value(), S("deepCopy"), clone, address(&maybeCloneClass)));
		}

		// Create copy ctors for derived versions of Maybe<T> and T.
		add(new (e) TemplateFn(new (e) Str(CTOR), fnPtr(e, &MaybeClassType::createCopy, this)));

		// Create assignment functions for derived versions of Maybe.
		add(new (e) TemplateFn(new (e) Str(S("=")), fnPtr(e, &MaybeClassType::createAssign, this)));

		return MaybeType::loadAll();
	}

	Named *MaybeClassType::createAssign(Str *name, SimplePart *part) {
		if (part->params->count() != 2)
			return null;

		if (part->params->at(0).type != this)
			return null;

		Value o = part->params->at(1);
		if (!isMaybe(o))
			return null;

		Value other = unwrapMaybe(o);
		if (!param().canStore(other))
			return null;

		Array<Value> *rv = new (this) Array<Value>(2, o);
		rv->at(0) = Value(this).asRef(true);
		return inlinedFunction(engine, Value(), S("="), rv, fnPtr(engine, &copyMaybeClass));
	}

	Named *MaybeClassType::createCopy(Str *name, SimplePart *part) {
		if (part->params->count() != 2)
			return null;

		if (part->params->at(0).type != this)
			return null;

		Value o = part->params->at(1);
		if (!isMaybe(o))
			return null;

		Value other = unwrapMaybe(o);

		if (!param().canStore(other))
			return null;

		Array<Value> *rv = new (this) Array<Value>(2, o.asRef(false));
		rv->at(0) = Value(this).asRef(true);

		Function *f = inlinedFunction(engine, Value(), CTOR, rv, fnPtr(engine, &copyMaybeClass));
		f->flags |= namedAutoCast;
		return f;
	}

	/**
	 * For values.
	 */

	static Size maybeValSize(Type *type) {
		return (type->size() + Size::sByte).aligned();
	}

	static GcType *allocTypeValue(Type *type) {
		Size size = maybeValSize(type);

		GcType *t = type->engine.gc.allocType(type->gcType());
		t->stride = size.current();
		return t;
	}

	MaybeValueType::MaybeValueType(Str *name, Type *param)
		: MaybeType(name, param, typeValue, maybeValSize(param), allocTypeValue(param)) {}

	code::TypeDesc *MaybeValueType::createTypeDesc() {
		using namespace code;
		TypeDesc *original = contained->typeDesc();

		if (PrimitiveDesc *p = as<PrimitiveDesc>(original)) {
			// Very simple. Struct with a primitive and a boolean.
			SimpleDesc *result = new (this) SimpleDesc(size(), 2);
			result->at(0) = p->v;
			result->at(1) = Primitive(primitive::integer, Size::sByte, Offset(original->size()));
			return result;
		} else if (SimpleDesc *s = as<SimpleDesc>(original)) {
			// Just add another primitive. We don't need a copy-ctor if the underlying type doesn't!
			SimpleDesc *result = new (this) SimpleDesc(size(), s->count() + 1);
			for (Nat i = 0; i < s->count(); i++)
				result->at(i) = s->at(i);

			result->at(s->count()) = Primitive(primitive::integer, Size::sByte, Offset(original->size()));
			return result;
		} else if (as<ComplexDesc>(original)) {
			// Complex description is the easiest actually... We just sort everything out in our copy-ctor.
			Ref ctor = engine.ref(Engine::rFnNull), dtor = engine.ref(Engine::rFnNull);
			if (Function *f = copyCtor())
				ctor = f->ref();
			if (Function *f = destructor())
				dtor = f->ref();

			return new (this) ComplexDesc(size(), ctor, dtor);
		} else {
			throw InternalError(L"Unknown type description found for " + ::toS(contained->identifier()));
		}
	}

	Bool MaybeValueType::loadAll() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value b = Value(StormInfo<Bool>::type(e));

		Array<Value> *r = new (e) Array<Value>(1, t.asRef(true));
		Array<Value> *rr = new (e) Array<Value>(2, t.asRef(true));
		Array<Value> *rp = new (e) Array<Value>(2, t.asRef(true));
		rp->at(1) = param().asRef(true);

		handle = &contained->handle();
		offset = boolOffset().current();

		add(inlinedFunction(e, Value(), CTOR, r, fnPtr(e, &MaybeValueType::initMaybe, this)));
		add(inlinedFunction(e, Value(), CTOR, rr, fnPtr(e, &MaybeValueType::copyMaybe, this)));
		add(inlinedFunction(e, t.asRef(true), S("="), rr, fnPtr(e, &MaybeValueType::copyMaybe, this)));
		add(inlinedFunction(e, b, S("empty"), r, fnPtr(e, &MaybeValueType::emptyMaybe, this)));
		add(inlinedFunction(e, b, S("any"), r, fnPtr(e, &MaybeValueType::anyMaybe, this)));

		// Cast constructor.
		Function *cast = inlinedFunction(e, Value(), CTOR, rp, fnPtr(e, &MaybeValueType::castMaybe, this));
		cast->flags |= namedAutoCast;
		add(cast);

		if (handle->toSFn) {
			add(inlinedFunction(e, Value(Str::stormType(e)), S("toS"), r, fnPtr(e, &MaybeValueType::toSMaybe, this)));

			Array<Value> *strBuf = new (e) Array<Value>(2, t);
			strBuf->at(1) = Value(StrBuf::stormType(e));
			add(inlinedFunction(e, Value(), S("toS"), strBuf, fnPtr(e, &MaybeValueType::toSMaybe, this)));
		}

		if (contained->deepCopyFn()) {
			Array<Value> *clone = new (e) Array<Value>(2, t);
			clone->at(1) = Value(CloneEnv::stormType(e));
			add(nativeFunction(e, Value(), S("deepCopy"), clone, fnPtr(e, &MaybeValueType::cloneMaybe, this)));
		}

		// Create copy ctors for derived versions of Maybe<T> and T.
		add(new (e) TemplateFn(new (e) Str(CTOR), fnPtr(e, &MaybeValueType::createCopy, this)));

		// Create assignment functions for derived versions of Maybe.
		add(new (e) TemplateFn(new (e) Str(S("=")), fnPtr(e, &MaybeValueType::createAssign, this)));

		return MaybeType::loadAll();
	}

	void MaybeValueType::initMaybe(InlineParams p) {
		using namespace code;
		p.allocRegs(0);
		*p.state->l << mov(byteRel(p.regParam(0), boolOffset()), byteConst(0));
	}

	void MaybeValueType::copyMaybe(InlineParams p) {
		using namespace code;
		p.allocRegs(0, 1);

		Reg dest = p.regParam(0);
		Reg src = p.regParam(1);

		Reg tmp = asSize(freeReg(dest, src), Size::sByte);


		// Store the result before we trash it.
		if (p.result->needed()) {
			*p.state->l << mov(p.result->location(p.state).v, dest);
		}

		Label empty = p.state->l->label();

		*p.state->l << mov(tmp, byteRel(src, boolOffset()));
		*p.state->l << mov(byteRel(dest, boolOffset()), tmp);
		*p.state->l << cmp(tmp, byteConst(0));
		*p.state->l << jmp(empty, ifEqual);

		// Copy.
		if (Value(contained).isBuiltIn()) {
			// Just move the value.
			Size sz = contained->size();
			*p.state->l << mov(xRel(sz, dest, Offset()), xRel(sz, src, Offset()));
		} else {
			Function *copyCtor = contained->copyCtor();
			if (!copyCtor)
				throw TypedefError(L"The type " + ::toS(contained->identifier()) + L" does not provide a copy constructor!");

			// Call the regular constructor! (TODO? Inline it?)
			*p.state->l << fnParam(engine.ptrDesc(), dest);
			*p.state->l << fnParam(engine.ptrDesc(), src);
			*p.state->l << fnCall(copyCtor->ref(), false);
		}

		*p.state->l << empty;
	}

	void MaybeValueType::castMaybe(InlineParams p) {
		using namespace code;
		p.allocRegs(0, 1);

		Reg dest = p.regParam(0);
		Reg src = p.regParam(1);

		// Store the result before we trash it.
		if (p.result->needed()) {
			*p.state->l << mov(p.result->location(p.state).v, dest);
		}

		*p.state->l << mov(byteRel(dest, boolOffset()), byteConst(1));

		// Copy.
		if (Value(contained).isBuiltIn()) {
			// Just move the value.
			Size sz = contained->size();
			*p.state->l << mov(xRel(sz, dest, Offset()), xRel(sz, src, Offset()));
		} else {
			Function *copyCtor = contained->copyCtor();
			if (!copyCtor) {
				// TODO: We could fall back to a memcpy implementation... We can't inline it,
				// however, as we don't know the exact size of the type (could be either 32- or 64-bit).
				throw TypedefError(L"The type " + ::toS(contained->identifier()) + L" does not provide a copy constructor!");
			}

			// Call the regular constructor! (TODO? Inline it?)
			*p.state->l << fnParam(engine.ptrDesc(), dest);
			*p.state->l << fnParam(engine.ptrDesc(), src);
			*p.state->l << fnCall(copyCtor->ref(), false);
		}
	}

	void MaybeValueType::emptyMaybe(InlineParams p) {
		using namespace code;
		if (!p.result->needed())
			return;

		p.allocRegs(0);
		*p.state->l << cmp(byteRel(p.regParam(0), boolOffset()), byteConst(0));
		*p.state->l << setCond(p.result->location(p.state).v, ifEqual);
	}

	void MaybeValueType::anyMaybe(InlineParams p) {
		using namespace code;
		if (!p.result->needed())
			return;

		p.allocRegs(0);
		*p.state->l << mov(p.result->location(p.state).v, byteRel(p.regParam(0), boolOffset()));
	}

	void MaybeValueType::toSMaybe(InlineParams p) {
		using namespace code;
		if (!p.result->needed())
			return;

		*p.state->l << fnParam(engine.ptrDesc(), typeRef());
		*p.state->l << fnParam(engine.ptrDesc(), p.param(0));
		*p.state->l << fnParam(engine.ptrDesc(), ptrConst(0));
		*p.state->l << fnCall(engine.ref(Engine::rMaybeToS), false, engine.ptrDesc(), p.result->location(p.state).v);
	}

	void MaybeValueType::toSMaybeBuf(InlineParams p) {
		using namespace code;
		if (!p.result->needed())
			return;

		*p.state->l << fnParam(engine.ptrDesc(), typeRef());
		*p.state->l << fnParam(engine.ptrDesc(), p.param(0));
		*p.state->l << fnParam(engine.ptrDesc(), p.param(1));
		*p.state->l << fnCall(engine.ref(Engine::rMaybeToS), false);
	}

	void *MaybeValueType::toSHelper(MaybeValueType *me, void *value, StrBuf *out) {
		StrBuf *to = out;
		if (!to)
			to = new (me) StrBuf();

		// Does this object represent 'null'?
		byte *data = (byte *)value;
		if (data[me->offset]) {
			(*me->handle->toSFn)(value, to);
		} else {
			*to << S("null");
		}

		// Were we called as 'toS()' or 'toS(StrBuf)'?
		if (out)
			return null;
		else
			return to->toS();
	}

	void MaybeValueType::cloneMaybe(InlineParams p) {
		using namespace code;

		Function *call = contained->deepCopyFn();
		if (!call)
			throw InternalError(L"The deep copy function was removed from " + ::toS(contained->identifier()));

		Label end = p.state->l->label();

		p.allocRegs(0);
		Reg dest = p.regParam(0);

		*p.state->l << cmp(byteRel(dest, boolOffset()), byteConst(0));
		*p.state->l << jmp(end, ifEqual);

		*p.state->l << fnParam(engine.ptrDesc(), dest);
		*p.state->l << fnParam(engine.ptrDesc(), p.param(1));
		*p.state->l << fnCall(contained->deepCopyFn()->ref(), true);

		*p.state->l << end;
	}

	Named *MaybeValueType::createAssign(Str *name, SimplePart *part) {
		if (part->params->count() != 2)
			return null;

		if (part->params->at(0).type != this)
			return null;

		Value o = part->params->at(1);
		if (!isMaybe(o))
			return null;

		Value other = unwrapMaybe(o);
		if (!param().canStore(other))
			return null;

		Array<Value> *rr = new (this) Array<Value>(2, o.asRef(true));
		rr->at(0) = Value(this).asRef(true);
		return inlinedFunction(engine, Value(), S("="), rr, fnPtr(engine, &MaybeValueType::copyMaybe, this));
	}

	Named *MaybeValueType::createCopy(Str *name, SimplePart *part) {
		if (part->params->count() != 2)
			return null;

		if (part->params->at(0).type != this)
			return null;

		Value o = part->params->at(1);
		if (!isMaybe(o))
			return null;

		Value other = unwrapMaybe(o);

		if (!param().canStore(other))
			return null;

		Array<Value> *rr = new (this) Array<Value>(2, o.asRef(true));
		rr->at(0) = Value(this).asRef(true);
		Function *f = inlinedFunction(engine, Value(), S("="), rr, fnPtr(engine, &MaybeValueType::copyMaybe, this));
		f->flags |= namedAutoCast;
		return f;
	}

}
