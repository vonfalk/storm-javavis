#include "stdafx.h"
#include "Variant.h"
#include "Type.h"
#include "Code/Listing.h"
#include "Engine.h"

namespace storm {

	MAYBE(Named *) CODECALL createVariantCtor(Str *name, SimplePart *part) {
		Engine &e = name->engine();
		if (part->params->count() != 2)
			return null;

		Value me = part->params->at(0);
		Value value = part->params->at(1);

		// Class types are already handled.
		if (!value.isValue())
			return null;
		if (!value.type)
			return null;

		me.ref = true;
		value.ref = true;

		using namespace code;
		Listing *l = new (e) Listing(true, e.voidDesc());
		Var pMe = l->createParam(e.ptrDesc());
		Var pVal = l->createParam(e.ptrDesc());

		*l << prolog();
		*l << fnParam(e.ptrDesc(), pMe);
		*l << fnParam(e.ptrDesc(), pVal);
		*l << fnParam(e.ptrDesc(), value.type->typeRef());
		*l << fnCall(e.ref(Engine::rCreateValVariant), false);
		*l << fnRet();

		Array<Value> *params = new (e) Array<Value>(2, me);
		params->at(1) = value;
		Function *fn = new (e) Function(Value(), name, params);
		fn->setCode(new (e) DynamicCode(l));
		fn->makePure();
		fn->make(fnAutoCast);
		return fn;
	}

	void addVariant(NameSet *to) {
		Engine &e = to->engine();
		Type *t = as<Type>(to);

		t->add(new (e) TemplateFn(new (e) Str(Type::CTOR), fnPtr(e, &createVariantCtor)));
	}

}
