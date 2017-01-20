#include "stdafx.h"
#include "TypeDtor.h"
#include "Type.h"

namespace storm {

	Bool STORM_FN needsDestructor(Type *type) {
		// A destructor is needed if any contained values have a destructor.
		Array<MemberVar *> *vars = type->variables();
		for (nat i = 0; i < vars->count(); i++) {
			Value t = vars->at(i)->type;
			if (!t.isValue())
				continue;

			if (t.type->destructor())
				return true;
		}

		return false;
	}

	TypeDefaultDtor::TypeDefaultDtor(Type *owner)
		: Function(Value(), new (owner) Str(Type::DTOR), new (owner) Array<Value>(1, thisPtr(owner))),
		  owner(owner) {

		setCode(new (this) LazyCode(fnPtr(engine(), &TypeDefaultDtor::generate, this)));
	}

	CodeGen *TypeDefaultDtor::generate() {
		using namespace code;

		CodeGen *t = new (this) CodeGen(runOn());
		Listing *l = t->l;

		Var me = l->createParam(valPtr());

		*l << prolog();

		// Destroy all value variables.
		Array<MemberVar *> *vars = owner->variables();
		for (nat i = 0; i < vars->count(); i++) {
			MemberVar *v = vars->at(i);
			if (!v->type.isValue())
				continue;

			Function *dtor = v->type.type->destructor();
			if (!dtor)
				continue;

			*l << mov(ptrA, me);
			*l << add(ptrA, ptrConst(v->offset()));
			*l << fnParam(ptrA);
			*l << fnCall(dtor->ref(), valPtr());
		}

		// Call super class' destructor (if any).
		Type *super = owner->super();
		if (super) {
			Function *dtor = super->destructor();
			if (dtor) {
				*l << fnParam(me);
				*l << fnCall(dtor->directRef(), valVoid());
			}
		}

		*l << epilog();
		*l << ret(valVoid());

		return t;
	}


}
