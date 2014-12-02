#include "stdafx.h"
#include "BSExpr.h"
#include "BSBlock.h"
#include "Lib/Int.h"
#include "Exception.h"
#include "function.h"

namespace storm {

	bs::Expr::Expr() {}

	Value bs::Expr::result() {
		return Value();
	}

	void bs::Expr::code(const GenState &to, code::Variable var) {
		assert(var == code::Variable::invalid);
	}

	bs::Constant::Constant(Int v) : cType(tInt), value(v) {}

	void bs::Constant::output(wostream &to) const {
		switch (cType) {
		case tInt:
			to << value << L"(I)";
			break;
		default:
			to << L"UNKNOWN";
			break;
		}
	}

	Value bs::Constant::result() {
		switch (cType) {
		case tInt:
			return Value(intType(engine()));
		default:
			TODO("Implement missing type");
			return Value();
		}
	}

	void bs::Constant::code(const GenState &s, code::Variable var) {
		using namespace code;

		if (var == code::Variable::invalid)
			return;

		switch (cType) {
		case tInt:
			s.to << mov(var, intConst(value));
			break;
		default:
			TODO("Implement missing type");
			break;
		}
	}

	bs::Constant *bs::intConstant(Auto<SStr> v) {
		return CREATE(Constant, v->engine(), v->v->v.toInt());
	}


	/**
	 * Operator
	 */

	bs::Operator::Operator(Auto<Block> block, Auto<Expr> lhs, Auto<SStr> op, Auto<Expr> rhs)
		: block(block.borrow()), lhs(lhs), rhs(rhs), op(op->v), fn(null) {
		TODO("Respect operator priority!");
	}

	Value bs::Operator::result() {
		Function *f = findFn();
		return f->result;
	}

	void bs::Operator::code(const GenState &s, code::Variable result) {
		using namespace code;
		Value r = this->result();
		// This code should be shared with general function calls!

		Variable to = result;
		if (to == Variable::invalid)
			to = s.frame.createVariable(s.block, r.size(), r.destructor());

		Value lhsType = lhs->result();
		Value rhsType = rhs->result();
		Variable lhsVar = s.frame.createVariable(s.block, lhsType.size(), lhsType.destructor());
		Variable rhsVar = s.frame.createVariable(s.block, rhsType.size(), rhsType.destructor());

		lhs->code(s, lhsVar);
		rhs->code(s, rhsVar);

		if (lhsType.refcounted())
			s.to << code::addRef(lhsVar);
		if (rhsType.refcounted())
			s.to << code::addRef(rhsVar);

		Function *f = findFn();
		vector<code::Value> params(2);
		params[0] = lhsVar;
		params[1] = rhsVar;
		f->genCode(s, params, result);
	}

	Function *bs::Operator::findFn() {
		if (fn)
			return fn;

		Value l = lhs->result();
		Value r = rhs->result();

		vector<Value> params(2);
		params[0] = l;
		params[1] = r;
		fn = as<Function>(block->scope->find(op->v, params));
		if (!fn)
			throw TypeError(pos, L"No overload for " + op->v + L" with " +
							::toS(l) + L" and " + ::toS(r) + L" found!");

		return fn;
	}

}
