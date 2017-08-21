#include "stdafx.h"
#include "Maybe.h"
#include "Engine.h"
#include "Package.h"
#include "Exception.h"
#include "Core/CloneEnv.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace storm {

	Type *createMaybe(Str *name, ValueArray *val) {
		if (val->count() != 1)
			return null;

		Value p = val->at(0);
		if (!p.isClass() && !p.isActor())
			return null;
		if (p.ref)
			return null;
		if (isMaybe(p))
			return null;

		return new (name) MaybeType(name, p.type);
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
		Type *found = as<Type>(to->find(new (e) SimplePart(new (e) Str(L"Maybe"), v)));
		if (!found)
			throw InternalError(L"Can not find the maybe type!");
		return Value(found);
	}

	static GcType *allocType(Engine &e) {
		GcType *t = e.gc.allocType(GcType::tArray, null, sizeof(void *), 1);
		t->offset[0] = 0;
		return t;
	}

	MaybeType::MaybeType(Str *name, Type *param)
		: Type(name,
			new (name) Array<Value>(1, Value(param)),
			typeClass | typeRawPtr | typeFinal,
			Size::sPtr,
			allocType(name->engine()),
			null),
		  contained(param) {}

	Value MaybeType::param() const {
		return Value(contained);
	}

	BasicTypeInfo::Kind MaybeType::builtInType() const {
		// Tell the world we're a pointer!
		return TypeKind::ptr;
	}

	static void initMaybe(InlineParams p) {
		using namespace code;
		*p.state->l << mov(ptrA, p.params->at(0));
		*p.state->l << mov(ptrRel(ptrA, Offset()), ptrConst(Offset()));
	}

	static void copyMaybe(InlineParams p) {
		using namespace code;
		*p.state->l << mov(ptrA, p.params->at(0));
		*p.state->l << mov(ptrRel(ptrA, Offset()), p.params->at(1));

		// See if we should return something (neccessary since we can be used as a constructor).
		if (p.result->needed()) {
			if (!p.result->suggest(p.state, p.params->at(0))) {
				*p.state->l << mov(p.result->location(p.state).v, p.params->at(0));
			}
		}
	}

	static void emptyMaybe(InlineParams p) {
		using namespace code;
		if (!p.result->needed())
			return;

		*p.state->l << cmp(p.params->at(0), ptrConst(Offset()));
		*p.state->l << setCond(p.result->location(p.state).v, ifEqual);
	}

	static void anyMaybe(InlineParams p) {
		using namespace code;
		if (!p.result->needed())
			return;

		*p.state->l << cmp(p.params->at(0), ptrConst(Offset()));
		*p.state->l << setCond(p.result->location(p.state).v, ifNotEqual);
	}

	static Str *maybeToS(EnginePtr e, RootObject *o) {
		if (!o)
			return new (e.v) Str(L"null");
		else
			return o->toS();
	}

	static void maybeToSBuf(RootObject *o, StrBuf *buf) {
		if (o)
			o->toS(buf);
		else
			*buf << L"null";
	}

	static void maybeClone(Object *o, CloneEnv *env) {
		if (o)
			o->deepCopy(env);
	}

	Bool MaybeType::loadAll() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value b = Value(StormInfo<Bool>::type(e));

		Array<Value> *v = new (e) Array<Value>(1, t);
		Array<Value> *r = new (e) Array<Value>(1, t.asRef(true));
		Array<Value> *rv = new (e) Array<Value>(2, t);
		rv->at(0) = t.asRef(true);

		add(inlinedFunction(e, Value(), CTOR, r, fnPtr(e, &initMaybe)));
		add(inlinedFunction(e, Value(), CTOR, rv, fnPtr(e, &copyMaybe)));
		add(inlinedFunction(e, t.asRef(true), S("="), rv, fnPtr(e, &copyMaybe)));
		add(inlinedFunction(e, b, S("empty"), v, fnPtr(e, &emptyMaybe)));
		add(inlinedFunction(e, b, S("any"), v, fnPtr(e, &anyMaybe)));

		add(nativeEngineFunction(e, Value(Str::stormType(e)), S("toS"), v, address(&maybeToS)));

		Array<Value> *strBuf = new (e) Array<Value>(2, t);
		strBuf->at(1) = Value(StrBuf::stormType(e));
		add(nativeFunction(e, Value(), S("toS"), strBuf, address(&maybeToSBuf)));

		if (!contained->isA(TObject::stormType(e))) {
			Array<Value> *clone = new (e) Array<Value>(2, t);
			clone->at(1) = Value(CloneEnv::stormType(e));
			add(nativeFunction(e, Value(), S("deepCopy"), clone, address(&maybeClone)));
		}

		// Create copy ctors for derived versions of Maybe<T> and T.
		add(new (e) TemplateFn(new (e) Str(CTOR), fnPtr(e, &MaybeType::createCopy, this)));

		// Create assignment functions for derived versions of Maybe.
		add(new (e) TemplateFn(new (e) Str(S("=")), fnPtr(e, &MaybeType::createAssign, this)));

		return Type::loadAll();
	}

	Named *MaybeType::createAssign(Str *name, SimplePart *part) {
		if (part->params->count() != 2)
			return null;

		Value o = part->params->at(1);
		if (!isMaybe(o))
			return null;

		Value other = unwrapMaybe(o);
		if (!param().canStore(other))
			return null;


		Array<Value> *rv = new (this) Array<Value>(2, o);
		rv->at(0) = Value(this).asRef(true);
		return inlinedFunction(engine, Value(), S("="), rv, fnPtr(engine, &copyMaybe));
	}

	Named *MaybeType::createCopy(Str *name, SimplePart *part) {
		if (part->params->count() != 2)
			return null;

		Value o = part->params->at(1);

		Value other = o;
		if (isMaybe(o))
			other = unwrapMaybe(o);

		if (!param().canStore(other))
			return null;


		Array<Value> *rv = new (this) Array<Value>(2, o.asRef(false));
		rv->at(0) = Value(this).asRef(true);
		Function *f = inlinedFunction(engine, Value(), CTOR, rv, fnPtr(engine, &copyMaybe));
		f->flags |= namedAutoCast;
		return f;
	}

}
