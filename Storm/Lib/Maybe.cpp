#include "stdafx.h"
#include "Maybe.h"
#include "Engine.h"
#include "Function.h"

namespace storm {

	Type *maybeType(Engine &e, const Value &type) {
		Auto<Name> tName = CREATE(Name, e);
		tName->add(L"core");
		tName->add(L"Maybe", vector<Value>(1, type));
		Type *r = as<Type>(e.scope()->find(tName));
		assert(r, "The maybe type was not found!");
		return r;
	}

	static Named *generateMaybe(Par<NamePart> part) {
		if (part->params.size() != 1)
			return null;

		const Value &param = part->params[0];
		if (!param.isClass())
			return null;
		if (param.ref)
			return null;
		if (isMaybe(param))
			return null;

		return CREATE(MaybeType, part, param);
	}

	void addMaybeTemplate(Par<Package> to) {
		Auto<Template> t = CREATE(Template, to, L"Maybe", simpleFn(&generateMaybe));
		to->add(t);
	}

	// Note: this is not always called since we are a built in type.
	static void createMaybe(Object **o) {
		*o = null;
	}

	static void initMaybe(Object **to, Object *src) {
		*to = src;
		src->addRef();
	}

	static Object *assignMaybe(Object **to, Object *from) {
		from->addRef();
		(*to)->release();
		(*to) = from;

		// For the return...
		(*to)->addRef();
		return *to;
	}

	static void deepCopyMaybe(Object *src, CloneEnv *env) {
		if (src)
			src->deepCopy(env);
	}

	static void destroyMaybe(Object *o) {
		o->release();
	}

	static bool emptyMaybe(Object *o) {
		return o == null;
	}

	static bool anyMaybe(Object *o) {
		return o != null;
	}

	static Str *anyToS(EnginePtr e, Object *o) {
		if (o == null)
			return CREATE(Str, e.v, L"null");
		else
			return o->toS();
	}

	// Disallow inheritance from this type, that will not work as the type system expects...
	MaybeType::MaybeType(const Value &param) :
		Type(L"Maybe", typeClass | typeFinal | typeRawPtr, vector<Value>(1, param), Size::sPtr),
		param(param) {}

	bool MaybeType::loadAll() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Value cloneEnv = Value::thisPtr(CloneEnv::stormType(e));

		add(steal(nativeFunction(e, Value(), CTOR, valList(1, t), &storm::createMaybe)));
		add(steal(nativeFunction(e, Value(), CTOR, valList(2, t, t), &storm::initMaybe)));

		// Ctor for the wrapped type.
		Auto<Function> f = nativeFunction(e, Value(), CTOR, valList(2, t, param), &storm::initMaybe);
		f->flags |= namedAutoCast;
		add(f);

		// Create copy ctors for derived versions of Maybe.
		add(steal(CREATE(Template, e, CTOR, memberFn(this, &MaybeType::createCopy))));

		// Create assignment functions for derived versions of Maybe.
		add(steal(CREATE(Template, e, L"=", memberFn(this, &MaybeType::createAssign))));

		add(steal(nativeFunction(e, t, L"=", valList(2, t.asRef(true), param), &storm::assignMaybe)));
		add(steal(nativeFunction(e, Value(), L"deepCopy", valList(2, t, cloneEnv), &storm::deepCopyMaybe)));
		add(steal(nativeFunction(e, Value(boolType(e)), L"empty", valList(1, t), &storm::emptyMaybe)));
		add(steal(nativeFunction(e, Value(boolType(e)), L"any", valList(1, t), &storm::anyMaybe)));
		add(steal(nativeEngineFunction(e, Value(Str::stormType(e)), L"toS", valList(1, t), &storm::anyToS)));
		add(steal(nativeDtor(e, this, &storm::destroyMaybe)));
		return Type::loadAll();
	}

	Named *MaybeType::createCopy(Par<NamePart> part) {
		if (part->params.size() < 2)
			return null;

		MaybeType *param = as<MaybeType>(part->params[1].type);
		if (!param)
			return null;

		if (!this->param.canStore(param->param))
			return null;

		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Named *n = nativeFunction(e, Value(), CTOR, valList(2, t, Value::thisPtr(param)), &storm::initMaybe);
		n->flags |= namedAutoCast;
		return n;
	}

	Named *MaybeType::createAssign(Par<NamePart> part) {
		if (part->params.size() < 2)
			return null;

		MaybeType *param = as<MaybeType>(part->params[1].type);
		if (!param)
			return null;

		if (!this->param.canStore(param->param))
			return null;

		Engine &e = engine;
		Value t = Value::thisPtr(this);
		return nativeFunction(e, t, L"=", valList(2, t.asRef(true), Value::thisPtr(param)), &storm::assignMaybe);
	}

	Bool isMaybe(Value v) {
		return as<MaybeType>(v.type) != null;
	}

	Value unwrapMaybe(Value v) {
		if (MaybeType *t = as<MaybeType>(v.type))
			return t->param;
		else
			return v;
	}

	Value wrapMaybe(Value v) {
		if (isMaybe(v))
			return v;
		return Value(maybeType(v.type->engine, v));
	}

}
