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

	void bs::NamedExpr::findTarget(Auto<Scope> scope, const Name &name, const SrcPos &pos) {
		Named *n = scope->find(name, params->values());
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
			return toLoad->result;
		return Value();
	}

	void bs::NamedExpr::code(const GenState &s, code::Variable to) {
		if (toExecute)
			callCode(s, to);
		else if (toLoad)
			loadCode(s, to);
		else
			assert(false);
	}

	void bs::NamedExpr::callCode(const GenState &s, code::Variable to) {
		using namespace code;
		const Value &r = toExecute->result;

		Variable resultVar = to;
		if (resultVar == Variable::invalid)
			resultVar = s.frame.createVariable(s.block, r.size(), r.destructor());

		vector<Value> values = params->values();
		vector<code::Value> vars(values.size());

		// Load parameters.
		for (nat i = 0; i < values.size(); i++) {
			Variable v = s.frame.createVariable(s.block, values[i].size(), values[i].destructor());
			params->expressions[i]->code(s, v);
			vars[i] = v;
		}

		// Increase refs for all parameters.
		for (nat i = 0; i < vars.size(); i++)
			if (values[i].refcounted())
				s.to << code::addRef(vars[i]);

		// Call!
		toExecute->genCode(s, vars, resultVar);
	}

	void bs::NamedExpr::loadCode(const GenState &s, code::Variable to) {
		using namespace code;

		if (to == Variable::invalid)
			return;

		s.to << mov(to, toLoad->var);
	}

}
