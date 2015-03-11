#include "stdafx.h"
#include "TypeCtor.h"
#include "Exception.h"
#include "Code/Instruction.h"
#include "Code/FnParams.h"
#include "Lib/CloneEnv.h"

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

	TypeDeepCopy::TypeDeepCopy(Type *type)
		: Function(Value(), L"deepCopy", valList(2, Value::thisPtr(type), Value(CloneEnv::type(type)))) {
		Function *before = null;
		Type *super = type->super();

		if (super) {
			// Find parent function.
			if (Function *f = as<Function>(super->find(name, params))) {
				before = f;
			} else {
				throw InternalError(L"Could not find the clone function of " + super->identifier());
			}
		}

		generateCode(type, before);
	}

	void TypeDeepCopy::generateCode(Type *type, Function *before) {
		using namespace code;

		Listing l;
		l << prolog();

		// Set up parameters. (always references, this ptr is always a reference of some kind).
		Variable me = l.frame.createPtrParam();
		Variable from = l.frame.createPtrParam();
		Variable env = l.frame.createPtrParam();

		// First, call the super class!
		if (before) {
			l << fnParam(me);
			l << fnParam(from);
			l << fnParam(env);
			l << fnCall(Ref(before->ref()), Size());
		}

		// Find the clone function for objects.
		Engine &e = engine();
		Named *n = e.package(L"core")->find(L"clone", valList(2, Value(Object::type(e)), Value(CloneEnv::type(e))));
		Function *f = as<Function>(n);
		if (!f)
			throw InternalError(L"Could not find the compiler generated clone function for objects!");
		Ref cloneObjFn(f->ref());

		// Find all refcounted objects and clone them. All values have already been done
		// by the copy-ctor!
		vector<Auto<TypeVar> > vars = type->variables();
		for (nat i = 0; i < vars.size(); i++) {
			TypeVar *v = vars[i].borrow();
			const Value &t = v->varType;

			if (t.ref)
				throw InternalError(L"References are not supported yet! For " + ::toS(*v));

			// Only need to do this with classes (FALSE)
			if (!t.isClass())
				continue;

			Offset offset = v->offset();
			l << mov(ptrA, from);
			l << fnParam(ptrRel(ptrA, offset));
			l << fnParam(env);
			l << fnCall(cloneObjFn, Size::sPtr);
			l << mov(ptrC, me);
			l << add(ptrC, intPtrConst(offset));
			l << code::releaseRef(ptrRel(ptrC));
			l << mov(ptrRel(ptrC), ptrA);
			l << code::addRef(ptrA);
		}

		l << epilog();
		l << ret(Size());

		PVAR(l);
		setCode(steal(CREATE(DynamicCode, this, l)));
	}

	// Find the deepCopy member in a Type.
	static Function *deepCopy(Type *in) {
		Named *n = in->find(L"deepCopy", valList(2, Value::thisPtr(in), Value(CloneEnv::type(in))));
		if (Function *f = as<Function>(n))
			return f;
		throw InternalError(L"The class " + in->identifier() + L" does not have a deepClone(CloneEnv) member.");
	}

	// Clone an object with an env.
	static Object *CODECALL cloneObjectEnv(Object *o, CloneEnv *env) {
		if (o == null)
			return null;

		Object *result = env->cloned(o);
		if (result)
			return result;

		Type *t = o->myType;
		const void *fn = t->copyCtorFn();
		result = createCopy(fn, o);

		result->deepCopy(env);
		return result;
	}

	// Clone an object.
	static Object *CODECALL cloneObject(Object *o) {
		if (o == null)
			// Hard work done!
			return null;

		Engine &e = o->engine();
		Auto<CloneEnv> env = CREATE(CloneEnv, e);
		return cloneObjectEnv(o, env.borrow());
	}

	static Named *stdClone(Engine &e, const String &name, const Value &t) {
		using namespace code;

		// Standard function for all objects!
		if (t.isClass()) {
			return nativeFunction(e, t, name, valList(1, t), &cloneObject);
		}

		if (t.isBuiltIn()) {
			// Built-in types are easy!
			Listing l;
			Variable par = l.frame.createParameter(t.size(), false);
			l << prolog();
			l << mov(asSize(ptrA, par.size()), par);
			l << epilog();
			l << ret(t.size());

			Auto<Function> result = CREATE(Function, e, t, name, valList(1, t));
			result->setCode(steal(CREATE(DynamicCode, e, l)));
			return result.ret();
		} else {
			// Value. Copy ctor + call deepCopy with an env.
			Listing l;
			Variable to = l.frame.createParameter(Size::sPtr, false);
			Variable from = l.frame.createParameter(Size::sPtr, false);
			l << prolog();
			l << fnParam(to);
			l << fnParam(from);
			l << fnCall(t.copyCtor(), Size());

			Type *envType = CloneEnv::type(e);
			Variable rawEnv = l.frame.createPtrVar(l.frame.root(), Ref(e.fnRefs.freeRef), freeOnException);
			Variable env = variable(l.frame, l.frame.root(), Value(envType));
			l << fnParam(Ref(envType->typeRef));
			l << fnCall(Ref(e.fnRefs.allocRef), Size::sPtr);
			l << mov(rawEnv, ptrA);
			l << fnParam(ptrA);
			l << fnCall(Ref(envType->defaultCtor()->ref()), Size());
			l << mov(env, ptrA);
			l << mov(rawEnv, intPtrConst(0));

			l << fnParam(to);
			l << fnParam(env);
			l << fnCall(Ref(deepCopy(t.type)->ref()), Size());
			l << mov(ptrA, to);
			l << epilog();
			l << ret(Size::sPtr);

			Auto<Function> result = CREATE(Function, e, t, name, valList(1, t.asRef()));
			result->setCode(steal(CREATE(DynamicCode, e, l)));
			return result.ret();
		}
	}

	static Named *stdClone(Engine &e, const String &name, const Value &t, const Value &env) {
		using namespace code;

		// Second parameter should be CloneEnv!
		if (env != Value(CloneEnv::type(e)))
			return null;

		if (t.isClass()) {
			return nativeFunction(e, t, name, valList(2, t, env), &cloneObjectEnv);
		}

		if (t.isBuiltIn()) {
			// Built-in types are easy!
			Listing l;
			Variable par = l.frame.createParameter(t.size(), false);
			Variable env = l.frame.createParameter(Size::sPtr, false);

			l << prolog();
			l << mov(asSize(ptrA, par.size()), par);
			l << epilog();
			l << ret(t.size());

			Auto<Function> result = CREATE(Function, e, t, name, valList(1, t));
			result->setCode(steal(CREATE(DynamicCode, e, l)));
			return result.ret();
		} else {
			// Value. Call the copy-ctor and then deepCopy.
			Listing l;
			Variable to = l.frame.createParameter(Size::sPtr, false);
			Variable from = l.frame.createParameter(Size::sPtr, false);
			Variable env = l.frame.createParameter(Size::sPtr, false);
			l << prolog();
			l << fnParam(to);
			l << fnParam(from);
			l << fnCall(t.copyCtor(), Size());
			l << fnParam(to);
			l << fnParam(env);
			l << fnCall(Ref(deepCopy(t.type)->ref()), Size());
			l << mov(ptrA, to);
			l << epilog();
			l << ret(Size::sPtr);

			Auto<Function> result = CREATE(Function, e, t, name, valList(1, t.asRef()));
			result->setCode(steal(CREATE(DynamicCode, e, l)));
			return result.ret();
		}
		return null;
	}

	Named *stdClone(Par<NamePart> param) {
		using namespace code;
		Engine &e = param->engine();

		if (param->params.size() == 1)
			return stdClone(e, param->name, param->params[0].asRef(false));

		if (param->params.size() == 2)
			return stdClone(e, param->name, param->params[0].asRef(false), param->params[1]);

		return null;
	}

	Template *cloneTemplate(Engine &to) {
		Template *t = CREATE(Template, to, L"clone");
		t->generateFn = simpleFn(stdClone);
		return t;
	}
}
