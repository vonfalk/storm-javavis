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

		return CREATE(MaybeType, part->engine(), param);
	}

	void addMaybeTemplate(Par<Package> to) {
		Auto<Template> t = CREATE(Template, to, L"Maybe", simpleFn(&generateMaybe));
		to->add(t);
	}

	// Note: this is not always called since we are a built in type.
	static void createMaybe(Object **o) {
		*o = null;
	}

	static void createMaybeCopy(Object **to, Object *src) {
		*to = src;
		src->addRef();
	}

	static void initMaybe(Object **to, Object *src) {
		*to = src;
		src->addRef();
	}

	static Object **assignMaybe(Object **to, Object *from) {
		(*to)->release();
		(*to) = from;
		from->addRef();
		return to;
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

	// Disallow inheritance from this type, that will not work as the type system expects...
	MaybeType::MaybeType(const Value &param) :
		Type(L"Maybe", typeClass | typeFinal | typeRawPtr, vector<Value>(1, param), Size::sPtr),
		param(param) {}

	bool MaybeType::loadAll() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Value cloneEnv = Value::thisPtr(CloneEnv::stormType(e));

		add(steal(nativeFunction(e, Value(), CTOR, valList(1, t), &storm::createMaybe)));
		add(steal(nativeFunction(e, Value(), CTOR, valList(2, t, t), &storm::createMaybeCopy)));
		add(steal(nativeFunction(e, Value(), CTOR, valList(2, t, param), &storm::initMaybe)));
		// Default assignment is used here.
		// add(steal(nativeFunction(e, t, L"=", valList(2, t, t), &storm::assignMaybe)));
		add(steal(nativeFunction(e, t, L"=", valList(2, t, param), &storm::assignMaybe)));
		add(steal(nativeFunction(e, Value(), L"deepCopy", valList(2, t, cloneEnv), &storm::deepCopyMaybe)));
		add(steal(nativeFunction(e, Value(boolType(e)), L"empty", valList(1, t), &storm::emptyMaybe)));
		add(steal(nativeFunction(e, Value(boolType(e)), L"any", valList(1, t), &storm::anyMaybe)));
		add(steal(nativeDtor(e, this, &storm::destroyMaybe)));
		return Type::loadAll();
	}

}
