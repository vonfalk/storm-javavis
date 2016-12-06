#include "stdafx.h"
#include "TypeCtor.h"
#include "Type.h"
#include "Exception.h"
#include "Core/Str.h"

namespace storm {
	using namespace code;

	TypeDefaultCtor::TypeDefaultCtor(Type *owner)
		: Function(Value(), new (owner) Str(Type::CTOR), new (owner) Array<Value>(1, thisPtr(owner))),
		  owner(owner) {

		setCode(new (this) LazyCode(fnPtr(engine(), &TypeDefaultCtor::generate, this)));
	}

	CodeGen *TypeDefaultCtor::generate() {
		CodeGen *t = new (this) CodeGen(runOn());
		Listing *l = t->to;

		Var me = l->createParam(valPtr());

		*l << prolog();

		Type *super = owner->super();
		if (super == TObject::stormType(engine())) {
			// Run the TObject constructor if possible.
			NamedThread *thread = runOn().thread;
			if (!thread)
				throw InternalError(L"Can not use TypeDefaultCtor with TObjects without specifying a thread. "
									L"Set a thread for " + ::toS(owner->identifier()) + L" and try again.");

			Array<Value> *params = new (this) Array<Value>();
			params->push(thisPtr(super));
			params->push(thisPtr(Thread::stormType(engine())));
			Function *ctor = as<Function>(super->find(Type::CTOR, params));
			if (!ctor)
				throw InternalError(L"Can not find the default constructor for TObject!");

			*l << fnParam(me);
			*l << fnParam(thread->ref());
			*l << fnCall(code::Ref(ctor->ref()), valPtr());
		} else if (super) {
			// Find and run the parent constructor.
			Function *ctor = super->defaultCtor();
			if (!ctor)
				throw InternalError(L"Can not use TypeDefaultCtor if no default constructor "
									L"for the parent type is present. See parent class for "
									+ ::toS(owner->identifier()));

			*l << fnParam(me);
			*l << fnCall(code::Ref(ctor->ref()), valPtr());
		} else {
			// No parent constructor to run.
		}

		// Initialize all variables...
		Array<MemberVar *> *vars = owner->variables();
		for (nat i = 0; i < vars->count(); i++) {
			MemberVar *v = vars->at(i);
			if (v->type.isValue()) {
				Function *ctor = v->type.type->defaultCtor();
				if (!ctor)
					throw InternalError(L"Can not use TypeDefaultCtor if a member does not have a default "
										L"constructor. See " + ::toS(owner->identifier()));
				*l << mov(ptrA, me);
				*l << add(ptrA, ptrConst(v->offset()));
				*l << fnParam(ptrA);
				*l << fnCall(code::Ref(ctor->ref()), valPtr());
			} else if (v->type.isClass()) {
				TODO(L"Implement me!");
			} else {
				// Built-in types.
			}
		}

		if (owner->typeFlags & typeClass) {
			// Set the VTable.
			*l << mov(ptrA, me);
			*l << mov(ptrRel(ptrA, Offset()), Ref(owner->vtable->ref()));
		}

		*l << mov(ptrA, me);
		*l << epilog();

		return t;
	}


	TypeCopyCtor::TypeCopyCtor(Type *owner)
		: Function(Value(), new (owner) Str(Type::CTOR), new (owner) Array<Value>(2, thisPtr(owner))),
		  owner(owner) {

		setCode(new (this) LazyCode(fnPtr(engine(), &TypeCopyCtor::generate, this)));
	}

	CodeGen *TypeCopyCtor::generate() {
		CodeGen *t = new (this) CodeGen(runOn());
		Listing *l = t->to;

		Var me = l->createParam(valPtr());
		Var src = l->createParam(valPtr());

		*l << prolog();

		if (Type *super = owner->super()) {
			Function *ctor = super->copyCtor();
			if (!ctor)
				throw InternalError(L"No copy constructor for " + ::toS(super->identifier())
									+ L", required from " + ::toS(owner->identifier()));

			*l << fnParam(me);
			*l << fnParam(src);
			*l << fnCall(Ref(ctor->ref()), valPtr());
		}

		// Copy all variables.
		Array<MemberVar *> *vars = owner->variables();
		for (nat i = 0; i < vars->count(); i++) {
			MemberVar *v = vars->at(i);

			*l << mov(ptrA, me);
			*l << mov(ptrC, src);
			if (v->type.isValue()) {
				Function *ctor = v->type.type->copyCtor();
				if (!ctor)
					throw InternalError(L"No copy constructor for " + ::toS(v->type.type->identifier())
										+ L", required from " + ::toS(owner->identifier()));

				*l << add(ptrA, ptrConst(v->offset()));
				*l << add(ptrC, ptrConst(v->offset()));
				*l << fnParam(ptrA);
				*l << fnParam(ptrC);
				*l << fnCall(Ref(ctor->ref()), valPtr());
			} else {
				// Pointer or built-in.
				*l << mov(xRel(v->type.size(), ptrA, v->offset()), xRel(v->type.size(), ptrC, v->offset()));
			}
		}

		if (owner->typeFlags & typeClass) {
			// Set the VTable.
			*l << mov(ptrA, me);
			*l << mov(ptrRel(ptrA, Offset()), Ref(owner->vtable->ref()));
		}

		*l << mov(ptrA, me);
		*l << epilog();

		return t;
	}


	TypeAssign::TypeAssign(Type *owner)
		: Function(thisPtr(owner), new (owner) Str(L"="), new (owner) Array<Value>(2, thisPtr(owner))),
		  owner(owner) {

		setCode(new (this) LazyCode(fnPtr(engine(), &TypeAssign::generate, this)));
	}

	CodeGen *TypeAssign::generate() {
		CodeGen *t = new (this) CodeGen(runOn());
		Listing *l = t->to;

		Var me = l->createParam(valPtr());
		Var src = l->createParam(valPtr());

		*l << prolog();

		if (Type *super = owner->super()) {
			Function *ctor = super->copyCtor();
			if (!ctor)
				throw InternalError(L"No assignment operator for " + ::toS(super->identifier())
									+ L", required from " + ::toS(owner->identifier()));

			*l << fnParam(me);
			*l << fnParam(src);
			*l << fnCall(Ref(ctor->ref()), valPtr());
		}

		// Copy all variables.
		Array<MemberVar *> *vars = owner->variables();
		for (nat i = 0; i < vars->count(); i++) {
			MemberVar *v = vars->at(i);

			*l << mov(ptrA, me);
			*l << mov(ptrC, src);
			if (v->type.isValue()) {
				Function *ctor = v->type.type->assignFn();
				if (!ctor)
					throw InternalError(L"No assignment operator for " + ::toS(v->type.type->identifier())
										+ L", required from " + ::toS(owner->identifier()));

				*l << add(ptrA, ptrConst(v->offset()));
				*l << add(ptrC, ptrConst(v->offset()));
				*l << fnParam(ptrA);
				*l << fnParam(ptrC);
				*l << fnCall(Ref(ctor->ref()), valPtr());
			} else {
				// Pointer or built-in.
				*l << mov(xRel(v->type.size(), ptrA, v->offset()), xRel(v->type.size(), ptrC, v->offset()));
			}
		}

		*l << mov(ptrA, me);
		*l << epilog();

		return t;
	}

	static Array<Value> *paramArray(Type *owner) {
		Array<Value> *v = new (owner) Array<Value>(2, thisPtr(owner));
		v->at(1) = thisPtr(CloneEnv::stormType(owner->engine));
		return v;
	}

	TypeDeepCopy::TypeDeepCopy(Type *owner)
		: Function(Value(), new (owner) Str(L"deepCopy"), paramArray(owner)),
		  owner(owner) {

		setCode(new (this) LazyCode(fnPtr(engine(), &TypeDeepCopy::generate, this)));
	}

	CodeGen *TypeDeepCopy::generate() {
		CodeGen *t = new (this) CodeGen(runOn());
		Listing *l = t->to;

		Var me = l->createParam(valPtr());
		Var env = l->createParam(valPtr());

		TODO(L"Implement deep copy properly!");

		return t;
	}

}
