#include "stdafx.h"
#include "TypeCtor.h"
#include "Exception.h"
#include "Code/Instruction.h"

namespace storm {

	TypeDefaultCtor::TypeDefaultCtor(Type *owner)
		: Function(Value(), Type::CTOR, vector<Value>(1, owner)) {

		Function *before = null;
		Type *super = owner->super();
		if (super) {
			// We may need to run another function first...
			Overload *ovl = as<Overload>(super->find(Name(Type::CTOR)));
			if (ovl)
				before = as<Function>(ovl->find(vector<Value>(1, super)));
			if (!before)
				throw InternalError(L"Did not find a default constructor in " + super->identifier());
		}

		generateCode(before);
	}

	void TypeDefaultCtor::generateCode(Function *before) {
		using namespace code;

		Listing l;
		Variable v = l.frame.createPtrParam(); // Refcounting here as well?

		l << prolog();

		if (before) {
			assert(!before->params[0].ref); // not implemented properly
			l << fnParam(v);
			l << fnCall(Ref(before->ref()), Size());
		}

		// TODO: Call constructors for any data members that requires it.

		l << epilog();
		l << ret(Size());

		setCode(CREATE(DynamicCode, this, l));
	}

}
