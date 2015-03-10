#include "stdafx.h"
#include "TypeCtor.h"
#include "Exception.h"
#include "Code/Instruction.h"
#include "Code/FnParams.h"

namespace storm {

	TypeDefaultCtor::TypeDefaultCtor(Type *owner)
		: Function(Value(), Type::CTOR, vector<Value>(1, Value::thisPtr(owner))) {

		Function *before = null;
		Type *super = owner->super();

		if (super) {
			// We may need to run another function first...
			before = super->defaultCtor();
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
			} else {
				// TODO: Initialize references using their constructor?
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
			before = super->copyCtor();
			if (!before)
				throw InternalError(L"Did not find a copy constructor in " + super->identifier());
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
				// TODO: Call clone!
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


	TypeAssignFn::TypeAssignFn(Type *owner)
		: Function(Value::thisPtr(owner), L"=", vector<Value>(2, Value::thisPtr(owner))) {

		Function *before = null;
		Type *super = owner->super();

		if (super) {
			// Find parent assignment operator.
			before = super->assignFn();
			if (!before)
				throw InternalError(L"Did not find an assignment operator in " + super->identifier());
		}

		generateCode(owner, before);
	}

	void TypeAssignFn::generateCode(Type *type, Function *before) {
		using namespace code;

		Listing l;
		Variable dest = l.frame.createPtrParam();
		Variable src = l.frame.createPtrParam();

		l << prolog();

		if (before) {
			l << fnParam(dest);
			l << fnParam(src);
			l << fnCall(Ref(before->ref()), Size::sPtr);
			if (before->result.refcounted())
				l << code::releaseRef(ptrA);
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
				l << fnCall(t.assignFn(), Size());
			} else {
				code::Value to = xRel(t.size(), ptrA, v->offset());
				code::Value from = xRel(t.size(), ptrC, v->offset());
				if (t.refcounted())
					l << code::releaseRef(to);
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

	/**
	 * Clone.
	 */

	// Clone an object.
	static Object *CODECALL cloneObject(Object *o) {
		if (o == null)
			// Hard work done!
			return null;

		Type *t = o->myType;
		const void *fn = t->copyCtorFn();

		byte data[code::FnParams::elemSize * 4]; // two paramets is enough..
		code::FnParams params(data);
		params.add(o);
		return createObj(t, fn, params);
	}

	Named *stdClone(Par<NamePart> param) {
		using namespace code;
		Engine &e = param->engine();

		if (param->params.size() != 1)
			return null;

		// We do not clone references.
		Value v = param->params[0].asRef(false);

		// If we need to clone an Object, we may as well generate that one directly.
		if (v.isClass()) {
			v = Value(Object::type(e));
			return nativeFunction(e, v, param->name, vector<Value>(1, v), &cloneObject);
		}

		if (v.isBuiltIn()) {
			// Built-in types are also easy!
			Listing l;
			Variable par = l.frame.createParameter(v.size(), false);

			l << prolog();
			l << mov(asSize(ptrA, v.size()), par);
			l << epilog();
			l << ret(v.size());

			Auto<Function> result = CREATE(Function, e, v, param->name, vector<Value>(1, v));
			result->setCode(steal(CREATE(DynamicCode, e, l)));
			return result.ret();
		} else {
			// It is a value. All we need to do is to call its copy-constructor. Note
			// that we take the value as reference here.
			Listing l;
			Variable res = l.frame.createParameter(Size::sPtr, false);
			Variable par = l.frame.createParameter(Size::sPtr, false);

			l << prolog();
			l << fnParam(res);
			l << fnParam(par);
			l << fnCall(v.copyCtor(), Size());
			l << mov(ptrA, res);
			l << epilog();
			l << ret(Size::sPtr);

			Auto<Function> result = CREATE(Function, e, v, param->name, vector<Value>(1, v.asRef()));
			result->setCode(steal(CREATE(DynamicCode, e, l)));
			return result.ret();
		}
	}

	Template *cloneTemplate(Engine &to) {
		Template *t = CREATE(Template, to, L"clone");
		t->generateFn = simpleFn(stdClone);
		return t;
	}
}
