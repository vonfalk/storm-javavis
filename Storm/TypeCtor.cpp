#include "stdafx.h"
#include "TypeCtor.h"
#include "Exception.h"
#include "Code/Instruction.h"

namespace storm {

	TypeDefaultCtor::TypeDefaultCtor(Type *owner)
		: Function(Value(), Type::CTOR, vector<Value>(1, Value::thisPtr(owner))) {

		Function *before = null;
		Type *super = owner->super();

		if (super) {
			// We may need to run another function first...
			Overload *ovl = as<Overload>(super->find(Name(Type::CTOR)));
			if (ovl)
				before = as<Function>(ovl->find(vector<Value>(1, Value::thisPtr(super))));
			if (!before)
				throw InternalError(L"Did not find a default constructor in " + super->identifier());
		}

		generateCode(owner, before);
	}

	void TypeDefaultCtor::generateCode(Type *type, Function *before) {
		using namespace code;

		Listing l;
		Variable dest = l.frame.createPtrParam();

		l << prolog();

		if (before) {
			l << fnParam(dest);
			l << fnCall(Ref(before->ref()), Size());
		}

		vector<Auto<TypeVar> > vars = type->variables();
		for (nat i = 0; i < vars.size(); i++) {
			TypeVar *v = vars[i].borrow();
			const Value &t = v->varType;
			if (t.isValue()) {
				l << mov(ptrA, dest);
				l << add(ptrA, intPtrConst(v->offset()));
				l << fnParam(ptrA);
				l << fnCall(t.defaultCtor(), Size());
			}
		}

		if (type->flags & typeClass) {
			// Set the vtable. (TODO: maybe a symbolic offset here?)
			l << mov(ptrA, dest);
			l << mov(ptrRel(ptrA), Ref(type->vtable.ref));
		}

		l << epilog();
		l << ret(Size());

		setCode(steal(CREATE(DynamicCode, this, l)));
	}

	TypeCopyCtor::TypeCopyCtor(Type *owner)
		: Function(Value(), Type::CTOR, vector<Value>(2, Value::thisPtr(owner))) {

		Function *before = null;
		Type *super = owner->super();

		if (super) {
			// Find parent copy-ctor.
			Overload *ovl = as<Overload>(super->find(Name(Type::CTOR)));
			if (ovl)
				before = as<Function>(ovl->find(vector<Value>(1, Value::thisPtr(super))));
			if (!before)
				throw InternalError(L"Did not find a default constructor in " + super->identifier());
		}

		generateCode(owner, before);
	}

	void TypeCopyCtor::generateCode(Type *type, Function *before) {
		using namespace code;

		Listing l;
		Variable dest = l.frame.createPtrParam();
		Variable src = l.frame.createPtrParam();

		l << prolog();

		if (before) {
			l << fnParam(dest);
			l << fnParam(src);
			l << fnCall(Ref(before->ref()), Size());
		}

		// Copy data members.
		vector<Auto<TypeVar> > vars = type->variables();
		for (nat i = 0; i < vars.size(); i++) {
			TypeVar *v = vars[i].borrow();
			const Value &t = v->varType;
			l << mov(ptrA, dest);
			l << mov(ptrC, src);
			if (t.isValue()) {
				l << add(ptrA, intPtrConst(v->offset()));
				l << add(ptrC, intPtrConst(v->offset()));
				l << fnParam(ptrA);
				l << fnParam(ptrC);
				l << fnCall(t.copyCtor(), Size());
			} else {
				code::Value to = xRel(t.size(), ptrA, v->offset());
				code::Value from = xRel(t.size(), ptrC, v->offset());
				l << mov(to, from);
				if (t.refcounted())
					l << code::addRef(to);
			}
		}

		if (type->flags & typeClass) {
			// Set the vtable.
			l << mov(ptrA, dest);
			l << mov(ptrRel(ptrA), Ref(type->vtable.ref));
		}

		l << epilog();
		l << ret(Size());

		setCode(steal(CREATE(DynamicCode, this, l)));
	}

	TypeDefaultDtor::TypeDefaultDtor(Type *owner)
		: Function(Value(), Type::DTOR, vector<Value>(1, Value::thisPtr(owner))) {

		Function *before = null;
		Type *super = owner->super();

		if (super) {
			// We may need to run another function first...
			before = super->destructor();
		}

		generateCode(owner, before);
	}

	void TypeDefaultDtor::generateCode(Type *type, Function *before) {
		using namespace code;

		Listing l;
		Variable dest = l.frame.createPtrParam();

		l << prolog();

		l << fnParam(intConst(128));
		l << fnCall(Ref(engine().dbgPrint), Size());

		vector<Auto<TypeVar> > vars = type->variables();
		for (nat i = 0; i < vars.size(); i++) {
			TypeVar *v = vars[i].borrow();
			const Value &t = v->varType;
			if (t.isValue()) {
				l << mov(ptrA, dest);
				l << add(ptrA, intPtrConst(v->offset()));
				l << fnParam(ptrA);
				l << fnCall(t.destructor(), Size());
			}
		}

		if (before) {
			l << fnParam(dest);
			l << fnCall(Ref(before->directRef()), Size());
		}

		l << epilog();
		l << ret(Size());

		setCode(steal(CREATE(DynamicCode, this, l)));
	}

}
