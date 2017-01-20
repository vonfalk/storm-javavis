#include "stdafx.h"
#include "Enum.h"
#include "Number.h"
#include "Engine.h"
#include "Core/Hash.h"
#include "Function.h"

namespace storm {

	Type *createEnum(Str *name, Size size, GcType *type) {
		return new (name) Enum(name, type, false);
	}

	Type *createBitmaskEnum(Str *name, Size size, GcType *type) {
		return new (name) Enum(name, type, true);
	}

	static GcType *createGcType(Engine &e) {
		return e.gc.allocType(GcType::tArray, null, Size::sInt.current(), 0);
	}

	using namespace code;

	static void enumAdd(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << mov(result, p.params->at(0));
			*p.state->l << or(result, p.params->at(1));
		}
	}

	static void enumSub(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << mov(result, p.params->at(1));
			*p.state->l << not(result);
			*p.state->l << and(result, p.params->at(0));
		}
	}

	static void enumOverlaps(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			// TODO: Use 'test' op-code if it is implemented.
			Var t = p.state->l->createVar(p.state->block, Size::sInt);
			*p.state->l << mov(t, p.params->at(0));
			*p.state->l << and(t, p.params->at(1));
			*p.state->l << setCond(result, ifNotEqual);
		}
	}

	Enum::Enum(Str *name, Bool bitmask)
		: Type(name, typeValue, Size::sInt, createGcType(name->engine()), null),
		  bitmask(bitmask) {

		if (engine.has(bootTemplates))
			lateInit();
	}


	Enum::Enum(Str *name, GcType *type, Bool bitmask)
		: Type(name, typeValue, Size::sInt, type, null),
		  bitmask(bitmask) {

		assert(type->stride == Size::sInt.current(), L"Check the size of your enums!");
		if (engine.has(bootTemplates))
			lateInit();
	}

	void Enum::lateInit() {
		Type::lateInit();

		if (engine.has(bootTemplates) && values == null)
			values = new (this) Array<EnumValue *>();
	}

	Bool Enum::loadAll() {
		Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);
		Value b(StormInfo<Bool>::type(engine));

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &numCopyCtor<Int>)));
		add(inlinedFunction(engine, Value(), L"=", rv, fnPtr(engine, &numAssign<Int>)));
		add(inlinedFunction(engine, b, L"==", vv, fnPtr(engine, &numCmp<ifEqual>)));
		add(inlinedFunction(engine, b, L"!=", vv, fnPtr(engine, &numCmp<ifNotEqual>)));

		if (bitmask) {
			add(inlinedFunction(engine, Value(this), L"+", vv, fnPtr(engine, &enumAdd)));
			add(inlinedFunction(engine, Value(this), L"-", vv, fnPtr(engine, &enumSub)));
			add(inlinedFunction(engine, b, L"&", vv, fnPtr(engine, &enumOverlaps)));
			add(inlinedFunction(engine, b, L"has", vv, fnPtr(engine, &enumOverlaps)));
		}

		add(nativeFunction(engine, Value(this), L"hash", v, &intHash));

		return Type::loadAll();
	}

	void Enum::add(Named *v) {
		Type::add(v);

		// Keep track of values ourselves as well!
		if (EnumValue *e = as<EnumValue>(v))
			values->push(e);
	}

	static void put(StrBuf *to, const wchar *val, bool &first) {
		if (!first)
			*to << L" + ";
		first = false;
		*to << val;
	}

	static void put(StrBuf *to, Str *val, bool &first) {
		put(to, val->c_str(), first);
	}

	void Enum::toString(StrBuf *to, Nat v) {
		// Note: this needs to be thread safe...
		bool first = true;

		for (Nat i = 0; i < values->count(); i++) {
			EnumValue *val = values->at(i);

			if (bitmask) {
				if (val->value & v) {
					put(to, val->name, first);
					v = v & ~val->value;
				}
			} else {
				if (val->value == v) {
					put(to, val->name, first);
					v = 0;
				}
			}
		}

		// Any remaining garbage?
		if (v != 0)
			put(to, L"<unknown>", first);
	}

	void Enum::toS(StrBuf *to) const {
		*to << L"enum " << identifier() << L" {\n";
		{
			Indent z(to);
			for (Nat i = 0; i < values->count(); i++) {
				EnumValue *v = values->at(i);
				if (bitmask) {
					*to << v->name << L" = " << hex(v->value) << L"\n";
				} else {
					*to << v->name << L" = " << v->value << L"\n";
				}
			}
		}
		*to << L"}";
	}

	EnumValue::EnumValue(Enum *owner, Str *name, Nat value)
		: Function(Value(owner), name, new (name) Array<Value>()),
		  value(value) {

		setCode(new (this) InlineCode(fnPtr(engine(), &EnumValue::generate, this)));
	}

	void EnumValue::toS(StrBuf *to) const {
		*to << identifier() << L" = " << value;
	}

	void EnumValue::generate(InlineParams p) {
		if (!p.result->needed())
			return;

		Operand result = p.result->location(p.state).v;
		*p.state->l << mov(result, natConst(value));
	}

	EnumOutput::EnumOutput() : Template(new (engine()) Str(L"<<")) {}

	MAYBE(Named *) EnumOutput::generate(SimplePart *part) {
		Array<Value> *params = part->params;
		if (params->count() != 2)
			return null;

		Value strBuf = Value(StrBuf::stormType(engine()));
		if (params->at(0).type != strBuf.type)
			return null;

		Value e = params->at(1).asRef(false);
		Enum *type = as<Enum>(e.type);
		if (!type)
			return null;

		Listing *l = new (this) Listing();
		Var out = l->createParam(valPtr());
		Var in = l->createParam(valInt());

		*l << prolog();

		*l << fnParam(type->typeRef());
		*l << fnParam(out);
		*l << fnParam(in);
		*l << fnCall(engine().ref(Engine::rEnumToS), valVoid());

		*l << mov(ptrA, out);
		*l << epilog();
		*l << ret(valPtr());

		return dynamicFunction(engine(), strBuf, L"<<", valList(engine(), 2, strBuf, e), l);
	}

}
