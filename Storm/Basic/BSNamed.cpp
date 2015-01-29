#include "stdafx.h"
#include "BSNamed.h"
#include "BSBlock.h"
#include "Exception.h"
#include "Function.h"

namespace storm {

	bs::Actual::Actual() {}

	vector<Value> bs::Actual::values() {
		vector<Value> v(expressions.size());
		for (nat i = 0; i < expressions.size(); i++) {
			v[i] = expressions[i]->result();
		}
		return v;
	}

	void bs::Actual::add(Par<Expr> e) {
		expressions.push_back(e);
	}

	void bs::Actual::addFirst(Par<Expr> e) {
		expressions.insert(expressions.begin(), e);
	}


	/**
	 * Function call.
	 */

	bs::FnCall::FnCall(Par<Function> toExecute, Par<Actual> params)
		: toExecute(toExecute), params(params) {}

	Value bs::FnCall::result() {
		return toExecute->result;
	}

	void bs::FnCall::code(const GenState &s, GenResult &to) {
		using namespace code;
		const Value &r = toExecute->result;

		// Note that toExecute->params may not be equal to params->values()
		// since some actual parameters may report that they can return a
		// reference to a value. However, we know that toExecute->params.canStore(params->values())
		// so it is OK to take toExecute->params directly.
		const vector<Value> &values = toExecute->params;
		vector<code::Value> vars(values.size());

		// Load parameters.
		for (nat i = 0; i < values.size(); i++) {
			GenResult gr(values[i], s.block);
			params->expressions[i]->code(s, gr);
			vars[i] = gr.location(s);
		}

		// Call!
		toExecute->genCode(s, vars, to);
	}


	/**
	 * Execute constructor.
	 */

	bs::CtorCall::CtorCall(Par<Function> ctor, Par<Actual> params) : ctor(ctor), params(params) {
		toCreate = ctor->params[0];
	}

	Value bs::CtorCall::result() {
		return toCreate.asRef(false);
	}

	void bs::CtorCall::code(const GenState &s, GenResult &to) {
		if (toCreate.isValue())
			createValue(s, to);
		else
			createClass(s, to);
	}

	void bs::CtorCall::createValue(const GenState &s, GenResult &to) {
		using namespace code;
		Engine &e = Object::engine();

		// Call the constructor with the address of our variable.
		vector<Value> values = ctor->params;
		vector<code::Value> vars(values.size());

		// Load parameters.
		TODO(L"Here, if the function we're calling takes a reference to something, but the params can not give a ref, "
			L"this breaks! Same goes for FnCall!");
		for (nat i = 1; i < values.size(); i++) {
			GenResult gr(values[i], s.block);
			params->expressions[i - 1]->code(s, gr);
			vars[i] = gr.location(s);
		}

		// This needs to be last, otherwise other generated code may overwrite it!
		if (to.type.ref) {
			// Should this really happen?
			DebugBreak();
			vars[0] = to.location(s);
		} else {
			code::Value srcVar = to.safeLocation(s, toCreate.asRef(false));
			vars[0] = code::Value(ptrA);
			s.to << lea(vars[0], srcVar);
		}

		// Call it!
		GenResult voidTo(Value(), s.block);
		ctor->genCode(s, vars, voidTo);
	}

	void bs::CtorCall::createClass(const GenState &s, GenResult &to) {
		using namespace code;

		Engine &e = Object::engine();

		code::Block subBlock = s.frame.createChild(s.block);
		s.to << begin(subBlock);
		GenState subState = { s.to, s.frame, subBlock };

		// Only free this one automatically on an exception. If there is no exception,
		// the memory will be owned by the object itself.
		Variable rawMemory = s.frame.createPtrVar(subBlock, Ref(e.freeRef), freeOnException);

		// Allocate memory into our temporary variable.
		s.to << fnParam(Ref(toCreate.type->typeRef));
		s.to << fnCall(Ref(e.allocRef), Size::sPtr);
		s.to << mov(rawMemory, ptrA);

		// Call the constructor:
		vector<Value> values = ctor->params;
		vector<code::Value> vars(values.size());

		// Load parameters.
		vars[0] = rawMemory;

		for (nat i = 1; i < values.size(); i++) {
			GenResult gr(values[i], subState.block);
			params->expressions[i - 1]->code(subState, gr);
			vars[i] = gr.location(subState);
		}

		// Call!
		GenResult voidTo(Value(), subBlock);
		ctor->genCode(subState, vars, voidTo);

		// Store our result.
		s.to << mov(to.location(subState), rawMemory);

		s.to << end(subBlock);
	}


	bs::CtorCall *bs::defaultCtor(const SrcPos &pos, Par<Type> t) {
		Overload *o = as<Overload>(t->find(Type::CTOR));
		if (!o)
			throw SyntaxError(pos, L"No default constructor for " + t->identifier());

		vector<Value> params(1, Value(t.borrow(), true));
		Function *f = as<Function>(o->find(params));
		if (!f)
			throw SyntaxError(pos, L"No default constructor for " + t->identifier());

		Auto<Actual> actual = CREATE(Actual, t);
		return CREATE(CtorCall, t, f, actual);
	}

	bs::CtorCall *bs::copyCtor(const SrcPos &pos, Par<Type> t, Par<Expr> src) {
		Overload *o = as<Overload>(t->find(Type::CTOR));
		if (!o)
			throw SyntaxError(pos, L"No copy-constructor for " + t->identifier());

		vector<Value> params(2, Value(t.borrow(), true));
		Function *f = as<Function>(o->find(params));
		if (!f)
			throw SyntaxError(pos, L"No copy-constructor for " + t->identifier());

		Auto<Actual> actual = CREATE(Actual, t);
		actual->add(src);
		return CREATE(CtorCall, t, f, actual);
	}


	/**
	 * Local variable.
	 */
	bs::LocalVarAccess::LocalVarAccess(Par<LocalVar> var) : var(var) {}

	Value bs::LocalVarAccess::result() {
		return var->result.asRef();
	}

	void bs::LocalVarAccess::code(const GenState &s, GenResult &to) {
		using namespace code;

		if (!to.needed())
			return;

		if (to.type.ref) {
			Variable v = to.location(s);
			s.to << lea(v, var->var);
		} else if (!to.suggest(s, var->var)) {
			Variable v = to.location(s);

			s.to << mov(v, var->var);
			if (var->result.refcounted())
				s.to << code::addRef(v);
		}
	}


	/**
	 * Member variable.
	 */
	bs::MemberVarAccess::MemberVarAccess(Par<Expr> member, Par<TypeVar> var) : member(member), var(var) {}

	Value bs::MemberVarAccess::result() {
		return var->varType.asRef();
	}

	void bs::MemberVarAccess::code(const GenState &s, GenResult &to) {
		using namespace code;

		if (!to.needed()) {
			// We still need to evaluate 'member', it may have side effects!
			GenResult mResult;
			member->code(s, mResult);
			return;
		}

		GenResult mResult(member->result().asRef(false), s.block);
		member->code(s, mResult);
		code::Value mVar = mResult.location(s);
		code::Value result = to.location(s);

		if (to.type.ref) {
			s.to << mov(result, mVar);
			s.to << add(result, intPtrConst(var->offset()));
		} else {
			// Not possible to suggest the already existing location, as it is
			// not a 'code::Variable'.
			s.to << mov(ptrA, mVar);
			s.to << add(ptrA, intPtrConst(var->offset()));
			s.to << mov(result, xRel(result.size(), ptrA));

			if (var->varType.refcounted())
				s.to << code::addRef(result);
		}
	}


	/**
	 * Assignment.
	 */
	bs::ClassAssign::ClassAssign(Par<Expr> to, Par<Expr> value) : to(to), value(value) {
		Value r = to->result();
		if ((r.type->flags & typeClass) != typeClass)
			throw TypeError(to->pos, L"The default assignment can not be used with other types than classes"
							L" at the moment. Please implement an assignment operator for your type.");
		if (!r.ref)
			throw TypeError(to->pos, L"Can not assign to a non-reference.");
	}

	Value bs::ClassAssign::result() {
		return to->result().asRef(false);
	}

	void bs::ClassAssign::code(const GenState &s, GenResult &to) {
		using namespace code;

		// Type to work with.
		Value t = this->to->result().asRef(false);

		// Target variable.
		GenResult lhs(t.asRef(true), s.block);
		this->to->code(s, lhs);
		code::Value targetAddr = lhs.location(s);

		// Compute RHS...
		GenResult rhs(t, s.block);
		value->code(s, rhs);

		// Erase the previous value.
		if (t.refcounted()) {
			s.to << code::mov(ptrA, targetAddr);
			s.to << code::releaseRef(ptrRel(ptrA));
			s.to << code::addRef(rhs.location(s));
		}
		s.to << code::mov(ptrRel(ptrA), rhs.location(s));

		// Do we need to return some value?
		if (!to.needed())
			return;

		if (to.type.ref) {
			if (!to.suggest(s, targetAddr)) {
				s.to << mov(to.location(s), targetAddr);
			}
		} else {
			s.to << mov(ptrA, targetAddr);
			s.to << mov(to.location(s), ptrRel(ptrA));
			s.to << code::addRef(to.location(s));
		}
	}


	/**
	 * Look up a proper action from a name and a set of parameters.
	 */
	namespace bs {
		static Expr *findCtor(Type *t, Par<Actual> actual, const SrcPos &pos);
		static Expr *findTarget(Named *n, Par<Expr> first, Par<Actual> actual, const SrcPos &pos);
		static Expr *findTargetThis(Par<Block> block, const Name &name,
									Par<Actual> params, const SrcPos &pos,
									Named *&candidate);
		static Expr *findTarget(Par<Block> block, const Name &name,
								const SrcPos &pos, Par<Actual> params,
								bool useThis);
	}

	// Find a constructor.
	static bs::Expr *bs::findCtor(Type *t, Par<Actual> actual, const SrcPos &pos) {
		vector<Value> params = actual->values();
		params.insert(params.begin(), Value::thisPtr(t));

		Overload *o = as<Overload>(t->find(Name(Type::CTOR)));
		if (!o)
			throw SyntaxError(pos, L"No constructor for " + t->identifier());

		Function *ctor = as<Function>(o->find(params));
		if (!ctor)
			throw SyntaxError(pos, L"No constructor " + t->identifier() + L"(" + join(params, L", ") + L")");

		return CREATE(CtorCall, t, ctor, actual);
	}


	// Helper to create the actual type, given something found.
	static bs::Expr *bs::findTarget(Named *n, Par<Expr> first, Par<Actual> actual, const SrcPos &pos) {
		if (!n)
			return null;

		if (n->name == Type::CTOR)
			throw SyntaxError(pos, L"Can not call a constructor by using __ctor. Use Type() instead.");
		if (n->name == Type::DTOR)
			throw SyntaxError(pos, L"Manual invocations of destructors are forbidden.");

		if (Function *f = as<Function>(n)) {
			if (first)
				actual->addFirst(first);
			return CREATE(FnCall, n, f, actual);
		}

		if (LocalVar *v = as<LocalVar>(n)) {
			assert(!first);
			return CREATE(LocalVarAccess, n, v);
		}

		if (TypeVar *v = as<TypeVar>(n)) {
			if (first)
				return CREATE(MemberVarAccess, n, first, v);
			else
				return CREATE(MemberVarAccess, n, actual->expressions.front(), v);
		}

		return null;
	}

	// Find a target assuming we should use the this-pointer.
	static bs::Expr *bs::findTargetThis(Par<Block> block, const Name &name,
										Par<Actual> params, const SrcPos &pos,
										Named *&candidate) {
		const Scope &scope = block->scope;

		LocalVar *thisVar = block->variable(L"this");
		if (!thisVar)
			return null;

		vector<Value> vals = params->values();
		vals.insert(vals.begin(), thisVar->result);
		Named *n = scope.find(name, vals);
		candidate = n;

		Auto<Expr> first = CREATE(LocalVarAccess, block, thisVar);
		Expr *e = findTarget(n, first, params, pos);
		if (e)
			return e;

		return null;
	}

	// Find whatever is meant by the 'name' in this context. Return suitable expression. If
	// 'useThis' is true, a 'this' pointer may be inserted as the first parameter.
	static bs::Expr *bs::findTarget(Par<Block> block, const Name &name,
									const SrcPos &pos, Par<Actual> params,
									bool useThis) {
		const Scope &scope = block->scope;

		if (Type *t = as<Type>(scope.find(name)))
			return findCtor(t, params, pos);

		// If we have a this-pointer, try to use it!
		Named *candidate = null;
		if (useThis)
			if (Expr *e = findTargetThis(block, name, params, pos, candidate))
				return e;

		Named *n = scope.find(name, params->values());

		if (Expr *e = findTarget(n, null, params, pos))
			return e;

		if (!n && !candidate)
			throw SyntaxError(pos, L"Can not find " + ::toS(name) + L"("
							+ join(params->values(), L", ") + L").");

		if (!n)
			n = candidate;

		throw TypeError(pos, ::toS(n) + L" is a " + n->myType->identifier() +
						L". Only functions, variables and constructors are supported.");
	}

	bs::Expr *bs::namedExpr(Par<Block> block, Par<SStr> name, Par<Actual> params) {
		return findTarget(block, Name(name->v->v), name->pos, params, true);
	}

	bs::Expr *bs::namedExpr(Par<Block> block, Par<SStr> name, Par<Expr> first, Par<Actual> params) {
		params->addFirst(first);
		return findTarget(block, Name(name->v->v), name->pos, params, false);
	}


	bs::Expr *bs::operatorExpr(Par<Block> block, Par<Expr> lhs, Par<SStr> m, Par<Expr> rhs) {
		Auto<Actual> actual = CREATE(Actual, block);
		actual->add(lhs);
		actual->add(rhs);
		return namedExpr(block, m, actual);
	}

	bs::Expr *bs::assignExpr(Par<Block> block, Par<Expr> lhs, Par<SStr> m, Par<Expr> rhs) {
		Value r = lhs->result();
		if (r.type && r.ref) {
			if (r.type->flags & typeClass) {
				return CREATE(ClassAssign, block, lhs, rhs);
			}
		}

		return operatorExpr(block, lhs, m, rhs);
	}

}
