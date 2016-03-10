#include "stdafx.h"
#include "TypeCtor.h"
#include "Exception.h"
#include "Type.h"
#include "Engine.h"
#include "CodeGen.h"
#include "Code/Instruction.h"
#include "Code/FnParams.h"
#include "Shared/CloneEnv.h"

namespace storm {

	TypeDefaultCtor::TypeDefaultCtor(Type *owner)
		: Function(Value(), Type::CTOR, vector<Value>(1, Value::thisPtr(owner))) {

		Function *before = null;
		NamedThread *beforeParam = null;
		Type *super = owner->super();

		if (super == TObject::stormType(engine())) {
			// Run the TObject constructor with a supplied parameter (if present).
			RunOn on = owner->runOn();
			if (on.state != RunOn::named)
				throw InternalError(L"No thread supplied to TObject. Mark " + owner->identifier()
									+ L" as running on a specific thread.");
			assert(on.thread, L"No thread supplied even though state == named");

			vector<Value> params(2);
			params[0] = Value::thisPtr(super);
			params[1] = Value::thisPtr(Thread::stormType(engine()));
			Auto<SimplePart> part = CREATE(SimplePart, this, Type::CTOR, params);
			Auto<Named> found = super->find(part);
			before = as<Function>(found.borrow());

			beforeParam = on.thread.borrow();
		} else if (super) {
			// We may need to run another function first...
			before = super->defaultCtor();
		}

		if (!before)
			throw InternalError(L"Did not find a default constructor in " + super->identifier());

		generateCode(owner, before, beforeParam);
	}

	void TypeDefaultCtor::generateCode(Type *type, Function *before, NamedThread *beforeParam) {
		using namespace code;

		Listing l;
		Variable dest = l.frame.createPtrParam();

		l << prolog();

		if (before) {
			l << fnParam(dest);
			if (beforeParam) {
				// If this happens, we want to pass the parent thread to a TObject constructor.
				l << fnParam(beforeParam->ref());
			}

			l << fnCall(before->ref(), retVoid());
		}

		vector<Auto<TypeVar> > vars = type->variables();
		for (nat i = 0; i < vars.size(); i++) {
			TypeVar *v = vars[i].borrow();
			const Value &t = v->varType;
			if (t.isValue()) {
				l << mov(ptrA, dest);
				l << add(ptrA, intPtrConst(v->offset()));
				l << fnParam(ptrA);
				l << fnCall(t.defaultCtor(), retVoid());
			} else {
				// TODO: Initialize references using their constructor?
			}
		}

		if (type->typeFlags & typeClass) {
			// Set the vtable. (TODO: maybe a symbolic offset here?)
			l << mov(ptrA, dest);
			l << mov(ptrRel(ptrA), type->vtable.ref);
		}

		l << epilog();
		l << ret(retVoid());

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
			l << fnCall(before->ref(), retVoid());
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
				l << fnCall(t.copyCtor(), retVoid());
			} else {
				// TODO: Call clone!
				code::Value to = xRel(t.size(), ptrA, v->offset());
				code::Value from = xRel(t.size(), ptrC, v->offset());
				l << mov(to, from);
				if (t.refcounted())
					l << code::addRef(to);
			}
		}

		if (type->typeFlags & typeClass) {
			// Set the vtable.
			l << mov(ptrA, dest);
			l << mov(ptrRel(ptrA), type->vtable.ref);
		}

		l << epilog();
		l << ret(retVoid());

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
			l << fnCall(before->ref(), retPtr());
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
				l << fnCall(t.assignFn(), retPtr());
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

		if (type->typeFlags & typeClass) {
			// Set the vtable.
			l << mov(ptrA, dest);
			l << mov(ptrRel(ptrA), type->vtable.ref);
		}

		l << epilog();
		l << ret(retVoid());

		setCode(steal(CREATE(DynamicCode, this, l)));
	}

	/**
	 * Clone.
	 */

	TypeDeepCopy::TypeDeepCopy(Type *type) :
		Function(Value(), L"deepCopy", valList(2, Value::thisPtr(type), Value(CloneEnv::stormType(type)))) {
		Auto<Function> before;
		Type *super = type->super();

		if (super) {
			// Find parent function.
			before = steal(super->findCpp(name, params)).as<Function>();
			if (!before) {
				throw InternalError(L"Could not find the clone function of " + super->identifier());
			}
		}

		generateCode(type, before.borrow());
	}

	void TypeDeepCopy::generateCode(Type *type, Function *before) {
		using namespace code;

		Listing l;
		l << prolog();

		// Set up parameters. (always references, this ptr is always a reference of some kind).
		Variable me = l.frame.createPtrParam();
		Variable env = l.frame.createPtrParam();

		// First, call the super class!
		if (before) {
			l << fnParam(me);
			l << fnParam(env);
			l << fnCall(before->directRef(), retVoid());
		}

		// Find the clone function for objects.
		Engine &e = engine();
		Package *core = e.package(L"core");

		// Find all refcounted objects and clone them. All values have already been done
		// by the copy-ctor!
		vector<Auto<TypeVar> > vars = type->variables();
		for (nat i = 0; i < vars.size(); i++) {
			TypeVar *v = vars[i].borrow();
			const Value &t = v->varType;

			if (t.ref)
				throw InternalError(L"References are not supported yet! For " + ::toS(*v));

			// We do not need to do the built-in types.
			if (t.isBuiltIn())
				continue;

			vector<Value> params = valList(2, Value::thisPtr(t.type), Value(CloneEnv::stormType(e)));
			Offset offset = v->offset();
			if (t.isValue()) {
				// Call 'deepCopy' directly.
				Auto<Function> fn = steal(t.type->findCpp(L"deepCopy", params)).as<Function>();
				if (!fn)
					throw InternalError(L"The type " + ::toS(t) + L" does not have a 'deepCopy' member.");

				l << mov(ptrA, me);
				l << add(ptrA, intPtrConst(offset));
				l << fnParam(ptrA);
				l << fnParam(env);
				l << fnCall(fn->ref(), retVoid());

			} else {
				// Find the clone function for us:
				Auto<Function> fn = steal(core->findCpp(L"clone", params)).as<Function>();
				if (!fn)
					throw InternalError(L"Failed to find the 'clone(T, CloneEnv) for T = " + ::toS(params[0]));

				l << mov(ptrA, me);
				l << fnParam(ptrRel(ptrA, offset));
				l << fnParam(env);
				l << fnCall(fn->ref(), retPtr());
				l << mov(ptrC, me);
				l << add(ptrC, intPtrConst(offset));
				l << code::releaseRef(ptrRel(ptrC));
				l << mov(ptrRel(ptrC), ptrA);
			}
		}

		l << epilog();
		l << ret(retVoid());

		setCode(steal(CREATE(DynamicCode, this, l)));
	}

	// Find the deepCopy member in a Type.
	Function *deepCopy(Type *in) {
		Auto<Named> n = in->findCpp(L"deepCopy", valList(2, Value::thisPtr(in), Value(CloneEnv::stormType(in))));
		if (Function *f = as<Function>(n.borrow()))
			// This is a little risky, but should be ok.
			return f;
		throw InternalError(L"The class " + in->identifier() + L" does not have a deepClone(CloneEnv) member.");
	}

	// Clone an object with an env.
	Object *CODECALL cloneObjectEnv(Object *o, CloneEnv *env) {
		if (o == null)
			return null;

		if (o->myType->isA(TObject::stormType(o->engine()))) {
			// No need to clone TObjects.
			o->addRef();
			return o;
		}

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
	Object *CODECALL cloneObject(Object *o) {
		if (o == null)
			// Hard work done!
			return null;

		if (o->myType->isA(TObject::stormType(o->engine()))) {
			// We do not need to clone TObjects.
			o->addRef();
			return o;
		}

		Engine &e = o->engine();
		Auto<CloneEnv> env = CREATE(CloneEnv, e);
		return cloneObjectEnv(o, env.borrow());
	}

	// "clone" threaded objects.
	static Object *CODECALL returnObject(Object *o) {
		o->addRef();
		return o;
	}

	static Object *CODECALL returnObjectEnv(Object *o, CloneEnv *env) {
		o->addRef();
		return o;
	}

	static Named *stdClone(Engine &e, const String &name, const Value &t) {
		using namespace code;

		// Standard function for all objects!
		if (t.isClass()) {
			if (t.type->runOn().state != RunOn::any)
				return nativeFunction(e, t, name, valList(1, t), &returnObject);
			else
				return nativeFunction(e, t, name, valList(1, t), &cloneObject);
		}

		if (t.isBuiltIn()) {
			// Built-in types are easy!
			Listing l;
			Variable par = l.frame.createParameter(t.size(), false);
			l << prolog();
			l << mov(asSize(ptrA, par.size()), par);
			l << epilog();
			l << ret(t.retVal());

			Auto<Function> result = CREATE(Function, e, t, name, valList(1, t));
			result->setCode(steal(CREATE(DynamicCode, e, l)));
			return result.ret();
		} else {
			// Value. Copy ctor + call deepCopy with an env.
			Auto<CodeGen> s = CREATE(CodeGen, e, RunOn());
			Listing &l = s->l->v;
			Variable to = l.frame.createParameter(Size::sPtr, false);
			Variable from = l.frame.createParameter(Size::sPtr, false);
			l << prolog();
			l << fnParam(to);
			l << fnParam(from);
			l << fnCall(t.copyCtor(), retVoid());

			Type *envType = CloneEnv::stormType(e);
			Variable env = variable(l.frame, l.frame.root(), Value(envType)).var();
			allocObject(s, envType->defaultCtor(), vector<code::Value>(), env);

			l << fnParam(to);
			l << fnParam(env);
			l << fnCall(deepCopy(t.type)->ref(), retVoid());
			l << mov(ptrA, to);
			l << epilog();
			l << ret(retPtr());

			Auto<Function> result = CREATE(Function, e, t, name, valList(1, t.asRef()));
			result->setCode(steal(CREATE(DynamicCode, e, l)));
			return result.ret();
		}
	}

	static Named *stdClone(Engine &e, const String &name, const Value &t, const Value &env) {
		using namespace code;

		// Second parameter should be CloneEnv!
		if (env != Value(CloneEnv::stormType(e)))
			return null;

		if (t.isClass()) {
			if (t.type->runOn().state != RunOn::any)
				return nativeFunction(e, t, name, valList(2, t, env), &returnObjectEnv);
			else
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
			l << ret(t.retVal());

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
			l << fnCall(t.copyCtor(), retVoid());
			l << fnParam(to);
			l << fnParam(env);
			l << fnCall(deepCopy(t.type)->ref(), retVoid());
			l << mov(ptrA, to);
			l << epilog();
			l << ret(retPtr());

			Auto<Function> result = CREATE(Function, e, t, name, valList(1, t.asRef()));
			result->setCode(steal(CREATE(DynamicCode, e, l)));
			return result.ret();
		}
		return null;
	}

	Named *generateStdClone(Par<SimplePart> part) {
		using namespace code;
		Engine &e = part->engine();

		Named *result = null;

		if (part->count() == 1)
			result = stdClone(e, part->name, part->param(0).asRef(false));

		else if (part->count() == 2)
			result = stdClone(e, part->name, part->param(0).asRef(false), part->param(1).asRef(false));

		if (result)
			result->flags = namedMatchNoInheritance;

		return result;
	}

	Template *cloneTemplate(Engine &to) {
		return CREATE(Template, to, L"clone", simpleFn(&generateStdClone));
	}
}
