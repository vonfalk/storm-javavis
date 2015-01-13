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


	/**
	 * Function/variable call.
	 */

	bs::NamedExpr::NamedExpr(Auto<Block> block, Auto<SStr> name, Auto<Actual> params)
		: toExecute(null), toLoad(null), params(params) {
		findTarget(block->scope, Name(name->v->v), name->pos);
	}

	void bs::NamedExpr::findTarget(const Scope &scope, const Name &name, const SrcPos &pos) {
		if (Type *t = as<Type>(scope.find(name))) {
			findCtor(t, pos);
			return;
		}

		Named *n = scope.find(name, params->values());

		if (!n) {
			TODO(L"Check this-pointer as well!");
			throw SyntaxError(pos, L"Can not find " + ::toS(name) + L"("
							+ join(params->values(), L", ") + L").");
		}

		if (n->name == Type::CTOR)
			throw SyntaxError(pos, L"Can not call a constructor outside a new-expression.");

		if (toExecute = as<Function>(n))
			return;

		if (toLoad = as<LocalVar>(n))
			return;

		throw TypeError(pos, ::toS(name) + L" is a " + n->myType->identifier() +
						L". Only functions, variables and constructors are supported.");
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
		return Value();
	}

	void bs::NamedExpr::code(const GenState &s, GenResult &to) {
		if (toCreate != Value())
			createCode(s, to);
		else if (toExecute)
			callCode(s, to);
		else if (toLoad)
			loadCode(s, to);
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

	void bs::NamedExpr::createCode(const GenState &s, GenResult &to) {
		using namespace code;

		Engine &e = Object::engine();

		assert(("Creating values is not implemented yet!", toCreate.type->flags & typeClass));

		Variable rawMemory = s.frame.createPtrVar(s.block, Ref(e.freeRef));

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
			GenResult gr(values[i], s.block);
			params->expressions[i - 1]->code(s, gr);
			vars[i] = gr.location(s);
		}

		// Increase refs for all parameters, except the not yet initialized object.
		for (nat i = 1; i < vars.size(); i++)
			if (values[i].refcounted())
				s.to << code::addRef(vars[i]);

		// Call!
		GenResult voidTo(Value(), s.block);
		toExecute->genCode(s, vars, voidTo);

		// Store our result.
		s.to << mov(to.location(s), rawMemory);

		// Since the constructor did not throw an exception, we can set the 'rawMemory'
		// to 'null' now since it would otherwise be freed.
		s.to << mov(rawMemory, intPtrConst(0));
	}

	bs::NamedExpr *bs::Operator(Auto<Block> block, Auto<Expr> lhs, Auto<SStr> m, Auto<Expr> rhs) {
		Auto<Actual> actual = CREATE(Actual, block);
		actual->add(lhs);
		actual->add(rhs);
		return CREATE(NamedExpr, block, block, m, actual);
	}

}
