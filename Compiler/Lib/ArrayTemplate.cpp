#include "stdafx.h"
#include "ArrayTemplate.h"
#include "Engine.h"
#include "Exception.h"
#include "Core/Str.h"

namespace storm {

	Type *createArray(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value param = params->at(0);
		if (param.ref)
			return null;

		return new (params) ArrayType(name, param.type);
	}

	Bool isArray(Value v) {
		return as<ArrayType>(v.type) != null;
	}

	Value unwrapArray(Value v) {
		if (ArrayType *t = as<ArrayType>(v.type))
			return t->param();
		else
			return v;
	}

	Value wrapArray(Value v) {
		if (!v.type)
			return v;
		if (v.ref)
			return v;

		Engine &e = v.type->engine;
		TemplateList *l = e.cppTemplate(ArrayId);
		NameSet *to = l->addTo();
		assert(to, L"Too early to use 'wrapArray'.");
		Type *found = as<Type>(to->find(new (e) SimplePart(new (e) Str(L"Array"), v)));
		if (!found)
			throw InternalError(L"Can not find the array type!");
		return Value(found);
	}

	ArrayType::ArrayType(Str *name, Type *contents) : Type(name, typeClass), contents(contents) {
		if (engine.has(bootTemplates))
			lateInit();

		setSuper(ArrayBase::stormType(engine));
	}

	void ArrayType::lateInit() {
		if (!params)
			params = new (engine) Array<Value>();
		if (params->count() < 1)
			params->push(Value(contents));

		Type::lateInit();
	}

	Value ArrayType::param() const {
		return Value(contents);
	}

}
