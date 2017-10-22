#include "stdafx.h"
#include "TypeCtor.h"
#include "Type.h"
#include "Exception.h"
#include "Package.h"
#include "Engine.h"
#include "Core/Str.h"

namespace storm {
	using namespace code;

	TypeDefaultCtor::TypeDefaultCtor(Type *owner)
		: Function(Value(), new (owner) Str(Type::CTOR), new (owner) Array<Value>(1, thisPtr(owner))),
		  owner(owner) {

		setCode(new (this) LazyCode(fnPtr(engine(), &TypeDefaultCtor::generate, this)));
	}

	Bool TypeDefaultCtor::pure() const {
		// Not a value: not pure!
		if ((owner->typeFlags & typeValue) != typeValue)
			return false;

		Array<MemberVar *> *v = owner->variables();
		for (Nat i = 0; i < v->count(); i++) {
			MemberVar *var = v->at(i);
			Value t = var->type;

			if (!t.type)
				continue;

			// Primitives are trivial to copy.
			if (!t.isBuiltIn())
				continue;

			// Classes and actors need initialization!
			if (!t.isValue())
				return false;

			Function *ctor = t.type->defaultCtor();
			if (!ctor)
				continue;

			if (!ctor->pure())
				return false;
		}

		return true;
	}

	CodeGen *TypeDefaultCtor::generate() {
		CodeGen *t = new (this) CodeGen(runOn());
		Listing *l = t->l;

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
			*l << fnCall(ctor->directRef(), valPtr());
		} else if (super) {
			// Find and run the parent constructor.
			Function *ctor = super->defaultCtor();
			if (!ctor)
				throw InternalError(L"Can not use TypeDefaultCtor if no default constructor "
									L"for the parent type is present. See parent class for "
									+ ::toS(owner->identifier()));

			*l << fnParam(me);
			*l << fnCall(ctor->directRef(), valPtr());
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
				*l << fnCall(ctor->ref(), valPtr());
			} else if (v->type.isHeapObj()) {
				Function *ctor = v->type.type->defaultCtor();
				if (!ctor)
					throw InternalError(L"Can not use TypeDefaultCtor if a member does not have a default "
										L"constructor. See " + ::toS(owner->identifier()));

				Var created = allocObject(t, ctor, new (this) Array<Operand>());
				*l << mov(ptrA, me);
				*l << mov(ptrRel(ptrA, v->offset()), created);
			} else {
				// Built-in types do not need initialization.
			}
		}

		if (owner->typeFlags & typeClass) {
			// Set the VTable.
			owner->vtable->insert(l, me);
		}

		*l << mov(ptrA, me);
		*l << epilog();
		*l << ret(valPtr());

		return t;
	}


	TypeCopyCtor::TypeCopyCtor(Type *owner)
		: Function(Value(), new (owner) Str(Type::CTOR), new (owner) Array<Value>(2, thisPtr(owner))),
		  owner(owner) {

		setCode(new (this) LazyCode(fnPtr(engine(), &TypeCopyCtor::generate, this)));
	}

	Bool TypeCopyCtor::pure() const {
		// Not a value: not pure!
		if ((owner->typeFlags & typeValue) != typeValue)
			return false;

		Array<MemberVar *> *v = owner->variables();
		for (Nat i = 0; i < v->count(); i++) {
			MemberVar *var = v->at(i);
			Value t = var->type;

			if (!t.type)
				continue;

			// Classes, actors and built-ins are trivial to copy!
			if (!t.isValue())
				continue;

			Function *ctor = t.type->copyCtor();
			if (!ctor)
				continue;

			if (!ctor->pure())
				return false;
		}

		return true;
	}

	CodeGen *TypeCopyCtor::generate() {
		CodeGen *t = new (this) CodeGen(runOn());
		Listing *l = t->l;

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
			*l << fnCall(ctor->ref(), valPtr());
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
				*l << fnCall(ctor->ref(), valPtr());
			} else {
				// Pointer or built-in.
				*l << mov(xRel(v->type.size(), ptrA, v->offset()), xRel(v->type.size(), ptrC, v->offset()));
			}
		}

		if (owner->typeFlags & typeClass) {
			// Set the VTable.
			owner->vtable->insert(l, me);
		}

		*l << mov(ptrA, me);
		*l << epilog();
		*l << ret(valPtr());

		return t;
	}


	TypeAssign::TypeAssign(Type *owner)
		: Function(thisPtr(owner), new (owner) Str(L"="), new (owner) Array<Value>(2, thisPtr(owner))),
		  owner(owner) {

		setCode(new (this) LazyCode(fnPtr(engine(), &TypeAssign::generate, this)));
	}

	Bool TypeAssign::pure() const {
		// Not a value: not pure!
		if ((owner->typeFlags & typeValue) != typeValue)
			return false;

		Array<MemberVar *> *v = owner->variables();
		for (Nat i = 0; i < v->count(); i++) {
			MemberVar *var = v->at(i);
			Value t = var->type;

			if (!t.type)
				continue;

			// Classes and actors are trivial to copy.
			if (!t.isValue())
				continue;

			Function *assign = t.type->assignFn();
			if (!assign)
				continue;

			if (!assign->pure())
				return false;
		}

		return true;
	}

	CodeGen *TypeAssign::generate() {
		CodeGen *t = new (this) CodeGen(runOn());
		Listing *l = t->l;

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
			*l << fnCall(ctor->ref(), valPtr());
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
				*l << fnCall(ctor->ref(), valPtr());
			} else {
				// Pointer or built-in.
				*l << mov(xRel(v->type.size(), ptrA, v->offset()), xRel(v->type.size(), ptrC, v->offset()));
			}
		}

		*l << mov(ptrA, me);
		*l << epilog();
		*l << ret(valPtr());

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
		Listing *l = t->l;

		Var me = l->createParam(valPtr());
		Var env = l->createParam(valPtr());

		*l << prolog();

		// Call the super-class (if possible).
		if (Type *super = owner->super()) {
			if (Function *before = super->deepCopyFn()) {
				*l << fnParam(me);
				*l << fnParam(env);
				*l << fnCall(before->directRef(), valVoid());
			}
		}

		Package *core = engine().package(S("core"));

		Array<MemberVar *> *vars = owner->variables();
		for (Nat i = 0; i < vars->count(); i++) {
			MemberVar *var = vars->at(i);
			Value type = var->type;


			if (type.isValue()) {
				// No need to copy, just call 'deepCopy'.
				Function *toCall = type.type->deepCopyFn();
				if (!toCall) {
					WARNING(L"No deepCopy function in " << type.type);
					continue;
				}

				*l << mov(ptrA, me);
				*l << add(ptrA, ptrConst(var->offset()));
				*l << fnParam(ptrA);
				*l << fnParam(env);
				*l << fnCall(toCall->ref(), valVoid());
			} else if (type.isClass()) {
				// Call the system-wide copy function for this type.
				Function *toCall = as<Function>(core->find(S("clone"), paramArray(type.type)));
				if (!toCall)
					throw InternalError(L"Can not find 'core.clone' for " + ::toS(type));

				*l << mov(ptrA, me);
				*l << fnParam(ptrRel(ptrA, var->offset()));
				*l << fnParam(env);
				*l << fnCall(toCall->ref(), valPtr());
				*l << mov(ptrC, me);
				*l << mov(ptrRel(ptrC, var->offset()), ptrA);
			} else {
				// Nothing needs to be done for actors or built-in types.
			}
		}

		*l << epilog();
		*l << ret(valPtr());

		return t;
	}

}
