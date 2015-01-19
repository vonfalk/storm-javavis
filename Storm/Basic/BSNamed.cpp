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

	void bs::Actual::add(Auto<Expr> e) {
		expressions.push_back(e);
	}

	void bs::Actual::addFirst(Auto<Expr> e) {
		expressions.insert(expressions.begin(), e);
	}


	/**
	 * Function call.
	 */

	bs::FnCall::FnCall(Auto<Function> toExecute, Auto<Actual> params)
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

		// Increase refs for all parameters.
		for (nat i = 0; i < vars.size(); i++)
			if (values[i].refcounted())
				s.to << code::addRef(vars[i]);

		// Call!
		toExecute->genCode(s, vars, to);
	}


	/**
	 * Execute constructor.
	 */

	bs::CtorCall::CtorCall(Auto<Function> ctor, Auto<Actual> params) : ctor(ctor), params(params) {
		toCreate = ctor->params[0];
	}

	Value bs::CtorCall::result() {
		return toCreate.asRef(false);
	}

	void bs::CtorCall::code(const GenState &s, GenResult &to) {
		using namespace code;

		Engine &e = Object::engine();

		assert(("Creating values is not implemented yet!", toCreate.type->flags & typeClass));

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
		const vector<Value> &values = ctor->params;
		vector<code::Value> vars(values.size());

		// Load parameters.
		vars[0] = rawMemory;

		for (nat i = 1; i < values.size(); i++) {
			GenResult gr(values[i], subState.block);
			params->expressions[i - 1]->code(subState, gr);
			vars[i] = gr.location(subState);
		}

		// Increase refs for all parameters, except the not yet initialized object.
		for (nat i = 1; i < vars.size(); i++)
			if (values[i].refcounted())
				s.to << code::addRef(vars[i]);

		// Call!
		GenResult voidTo(Value(), subBlock);
		ctor->genCode(subState, vars, voidTo);

		// Store our result.
		s.to << mov(to.location(subState), rawMemory);

		s.to << end(subBlock);
	}


	/**
	 * Local variable.
	 */
	bs::LocalVarAccess::LocalVarAccess(Auto<LocalVar> var) : var(var) {}

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

			if (var->result.refcounted())
				s.to << code::addRef(v);
			s.to << mov(v, var->var);
		}
	}


	/**
	 * Member variable.
	 */
	bs::MemberVarAccess::MemberVarAccess(Auto<Expr> member, Auto<TypeVar> var) : member(member), var(var) {}

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
	 * Look up a proper action from a name and a set of parameters.
	 */
	namespace bs {
		static Expr *findCtor(Type *t, Auto<Actual> actual, const SrcPos &pos);
		static Expr *findTarget(Named *n, Auto<Expr> first, Auto<Actual> actual, const SrcPos &pos);
		static Expr *findTargetThis(Auto<Block> block, const Name &name,
									Auto<Actual> params, const SrcPos &pos,
									Named *&candidate);
		static Expr *findTarget(Auto<Block> block, const Name &name,
								const SrcPos &pos, Auto<Actual> params,
								bool useThis);
	}

	// Find a constructor.
	static bs::Expr *bs::findCtor(Type *t, Auto<Actual> actual, const SrcPos &pos) {
		vector<Value> params = actual->values();
		params.insert(params.begin(), Value(t)); // this-ptr for the created type.

		Overload *o = as<Overload>(t->find(Name(Type::CTOR)));
		if (!o)
			throw SyntaxError(pos, L"No constructor for " + t->identifier());

		Function *ctor = as<Function>(o->find(params));
		if (!ctor)
			throw SyntaxError(pos, L"No constructor (" + join(params, L", ") + L")");

		return CREATE(CtorCall, t, capture(ctor), actual);
	}


	// Helper to create the actual type, given something found.
	static bs::Expr *bs::findTarget(Named *n, Auto<Expr> first, Auto<Actual> actual, const SrcPos &pos) {
		if (!n)
			return null;

		if (n->name == Type::CTOR)
			throw SyntaxError(pos, L"Can not call a constructor by using __ctor. Use Foo() instead.");

		if (Function *f = as<Function>(n)) {
			if (first)
				actual->addFirst(first);
			return CREATE(FnCall, n, capture(f), actual);
		}

		if (LocalVar *v = as<LocalVar>(n)) {
			assert(!first);
			return CREATE(LocalVarAccess, n, capture(v));
		}

		if (TypeVar *v = as<TypeVar>(n)) {
			if (first)
				return CREATE(MemberVarAccess, n, first, capture(v));
			else
				return CREATE(MemberVarAccess, n, actual->expressions.front(), capture(v));
		}

		return null;
	}

	// Find a target assuming we should use the this-pointer.
	static bs::Expr *bs::findTargetThis(Auto<Block> block, const Name &name,
										Auto<Actual> params, const SrcPos &pos,
										Named *&candidate) {
		const Scope &scope = block->scope;

		LocalVar *thisVar = block->variable(L"this");
		if (!thisVar)
			return null;

		vector<Value> vals = params->values();
		vals.insert(vals.begin(), thisVar->result);
		Named *n = scope.find(name, vals);
		candidate = n;

		Auto<Expr> first = CREATE(LocalVarAccess, block, capture(thisVar));
		Expr *e = findTarget(n, first, params, pos);
		if (e)
			return e;

		return null;
	}

	// Find whatever is meant by the 'name' in this context. Return suitable expression. If
	// 'useThis' is true, a 'this' pointer may be inserted as the first parameter.
	static bs::Expr *bs::findTarget(Auto<Block> block, const Name &name,
									const SrcPos &pos, Auto<Actual> params,
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

	bs::Expr *bs::namedExpr(Auto<Block> block, Auto<SStr> name, Auto<Actual> params) {
		return findTarget(block, Name(name->v->v), name->pos, params, true);
	}

	bs::Expr *bs::namedExpr(Auto<Block> block, Auto<SStr> name, Auto<Expr> first, Auto<Actual> params) {
		params->addFirst(first);
		return findTarget(block, Name(name->v->v), name->pos, params, false);
	}


	bs::Expr *bs::operatorExpr(Auto<Block> block, Auto<Expr> lhs, Auto<SStr> m, Auto<Expr> rhs) {
		Auto<Actual> actual = CREATE(Actual, block);
		actual->add(lhs);
		actual->add(rhs);
		return namedExpr(block, m, actual);
	}

}
