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
	 * Function/variable call.
	 */

	bs::NamedExpr::NamedExpr(Auto<Block> block, Auto<SStr> name, Auto<Actual> params)
		: toExecute(null), toLoad(null), params(params) {
		findTarget(block, Name(name->v->v), name->pos, true);
	}

	bs::NamedExpr::NamedExpr(Auto<Block> block, Auto<SStr> name, Auto<Expr> first, Auto<Actual> params)
		: toExecute(null), toLoad(null), params(params) {
		params->addFirst(first);
		findTarget(block, Name(name->v->v), name->pos, false);
	}

	void bs::NamedExpr::findTarget(Auto<Block> block, const Name &name, const SrcPos &pos, bool useThis) {
		const Scope &scope = block->scope;

		if (Type *t = as<Type>(scope.find(name))) {
			findCtor(t, pos);
			return;
		}

		// If we have a this-pointer, try to use it!
		Named *candidateN = null;
		if (useThis && findTargetThis(block, name, pos, candidateN))
			return;

		Named *n = scope.find(name, params->values());

		if (findTarget(n, pos))
			return;

		if (!n && !candidateN) {
			throw SyntaxError(pos, L"Can not find " + ::toS(name) + L"("
							+ join(params->values(), L", ") + L").");
		}

		if (!n)
			n = candidateN;

		throw TypeError(pos, ::toS(n) + L" is a " + n->myType->identifier() +
						L". Only functions, variables and constructors are supported.");
	}

	bool bs::NamedExpr::findTarget(Named *n, const SrcPos &pos) {
		if (!n)
			return false;

		if (n->name == Type::CTOR)
			throw SyntaxError(pos, L"Can not call a constructor by using __ctor. Use Foo() instead.");

		if (toExecute = as<Function>(n))
			return true;

		if (toLoad = as<LocalVar>(n))
			return true;

		if (toAccess = as<TypeVar>(n))
			return true;

		return false;
	}

	bool bs::NamedExpr::findTargetThis(Auto<Block> block, const Name &name, const SrcPos &pos, Named *&candidate) {
		const Scope &scope = block->scope;

		LocalVar *thisVar = block->variable(L"this");
		if (!thisVar)
			return false;

		vector<Value> vals = params->values();
		vals.insert(vals.begin(), thisVar->result);
		Named *n = scope.find(name, vals);
		candidate = n;

		if (!findTarget(n, pos))
			return false;

		// Ok, now we need to modify the parameters by adding a reference to 'this'.
		Auto<Actual> subParams = CREATE(Actual, this);
		Auto<SStr> nThis = CREATE(SStr, this, L"this");
		Auto<NamedExpr> sub = CREATE(NamedExpr, this, block, nThis, subParams);
		params->addFirst(sub);

		return true;
	}


	void bs::NamedExpr::findCtor(Type *t, const SrcPos &pos) {
		vector<Value> params = this->params->values();
		params.insert(params.begin(), Value(t)); // this-ptr for the created type.

		Overload *o = as<Overload>(t->find(Name(Type::CTOR)));
		if (!o)
			throw SyntaxError(pos, L"No constructor for " + t->identifier());

		Function *ctor = as<Function>(o->find(params));
		if (!ctor)
			throw SyntaxError(pos, L"No constructor (" + join(params, L", ") + L")");

		toCreate = Value(t);
		toExecute = ctor;
	}

	Value bs::NamedExpr::result() {
		if (toCreate != Value())
			return toCreate;
		if (toExecute)
			return toExecute->result;
		if (toLoad)
			return toLoad->result.asRef();
		if (toAccess)
			return toAccess->varType.asRef();
		assert(false);
		return Value();
	}

	void bs::NamedExpr::code(const GenState &s, GenResult &to) {
		if (toCreate != Value())
			createCode(s, to);
		else if (toExecute)
			callCode(s, to);
		else if (toLoad)
			loadCode(s, to);
		else if (toAccess)
			accessCode(s, to);
		else
			assert(false);
	}

	void bs::NamedExpr::callCode(const GenState &s, GenResult &to) {
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

	void bs::NamedExpr::loadCode(const GenState &s, GenResult &to) {
		using namespace code;

		if (!to.needed())
			return;

		if (to.type.ref) {
			Variable v = to.location(s);
			s.to << lea(v, toLoad->var);
		} else if (!to.suggest(s, toLoad->var)) {
			Variable v = to.location(s);

			if (toLoad->result.refcounted())
				s.to << code::addRef(v);
			s.to << mov(v, toLoad->var);
		}
	}

	void bs::NamedExpr::accessCode(const GenState &s, GenResult &to) {
		using namespace code;
		assert(params->expressions.size() == 1);

		if (!to.needed())
			return;

		// Get the 'this' ptr.
		Auto<Expr> tExpr = params->expressions[0];
		GenResult tResult(tExpr->result().asRef(false), s.block);
		tExpr->code(s, tResult);
		code::Value tVar = tResult.location(s);
		code::Value result = to.location(s);

		if (to.type.ref) {
			s.to << mov(result, tVar);
			s.to << add(result, intPtrConst(toAccess->offset()));
		} else {
			// We need to copy the variable, we can not suggest a temporary location.
			s.to << mov(ptrA, tVar);
			s.to << add(ptrA, intPtrConst(toAccess->offset()));
			s.to << mov(result, xRel(result.size(), ptrA));
		}
	}

	void bs::NamedExpr::createCode(const GenState &s, GenResult &to) {
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
		const vector<Value> &values = toExecute->params;
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
		toExecute->genCode(subState, vars, voidTo);

		// Store our result.
		s.to << mov(to.location(subState), rawMemory);

		s.to << end(subBlock);
	}

	bs::NamedExpr *bs::Operator(Auto<Block> block, Auto<Expr> lhs, Auto<SStr> m, Auto<Expr> rhs) {
		Auto<Actual> actual = CREATE(Actual, block);
		actual->add(lhs);
		actual->add(rhs);
		return CREATE(NamedExpr, block, block, m, actual);
	}

}
