#include "stdafx.h"
#include "MaybeTemplate.h"
#include "Engine.h"
#include "Package.h"
#include "Core/Str.h"

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
		assert(to);
		Named *found = to->find(new (e) SimplePart(new (e) Str(L"Maybe"), v));
		Type *t = as<Type>(found);
		assert(t);
		return Value(t);
	}

	static GcType *allocType(Engine &e) {
		GcType *t = e.gc.allocType(GcType::tArray, null, sizeof(void *), 1);
		t->offset[0] = 0;
		return t;
	}

	MaybeType::MaybeType(Str *name, Type *param) :
		Type(name,
			new (name) Array<Value>(1, Value(param)),
			typeValue | typeFinal,
			Size::sPtr,
			allocType(name->engine()),
			null) {

		TODO(L"Fix the implementation!");
	}

	Value MaybeType::param() const {
		return Value(contained);
	}

	BasicTypeInfo::Kind MaybeType::builtInType() const {
		// Tell the world we're a pointer!
		return BasicTypeInfo::ptr;
	}

}
