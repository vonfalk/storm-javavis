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
		Named *n = scope.find(name, params->values());
		if (!n)
			throw SyntaxError(pos, L"Can not find " + ::toS(name) + L"("
							+ join(params->values(), L", ") + L").");

		toExecute = as<Function>(n);
		toLoad = as<LocalVar>(n);

		if (!toExecute && !toLoad) {
			throw TypeError(pos, ::toS(name) + L" is of an unsupported type.");
		}
	}

	Value bs::NamedExpr::result() {
		if (toExecute)
			return toExecute->result;
		if (toLoad)
			return toLoad->result.asRef();
		return Value();
	}

	void bs::NamedExpr::code(const GenState &s, GenResult &to) {
		if (toExecute)
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

	bs::NamedExpr *bs::Operator(Auto<Block> block, Auto<Expr> lhs, Auto<SStr> m, Auto<Expr> rhs) {
		Auto<Actual> actual = CREATE(Actual, block);
		actual->add(lhs);
		actual->add(rhs);
		return CREATE(NamedExpr, block, block, m, actual);
	}

}
