#include "stdafx.h"
#include "ExprResult.h"
#include "Core/StrBuf.h"

namespace storm {

	ExprResult::ExprResult() : returns(true) {}

	ExprResult::ExprResult(Value value) : value(value), returns(true) {}

	ExprResult::ExprResult(bool any) : returns(any) {}

	Value ExprResult::type() const {
		if (returns)
			return value;
		else
			return Value();
	}

	Bool ExprResult::any() const {
		return returns;
	}

	Bool ExprResult::empty() const {
		return !returns;
	}

	Bool ExprResult::operator ==(const ExprResult &o) const {
		return returns == o.returns && value == o.value;
	}

	Bool ExprResult::operator !=(const ExprResult &o) const {
		return !(*this == o);
	}

	ExprResult ExprResult::asRef(Bool r) const {
		if (!any())
			return *this;
		return ExprResult(value.asRef(r));
	}

	wostream &operator <<(wostream &to, const ExprResult &r) {
		if (r.any()) {
			to << r.type();
		} else {
			to << "<nothing>";
		}
		return to;
	}

	StrBuf &operator <<(StrBuf &to, ExprResult r) {
		if (r.any()) {
			to << r.type();
		} else {
			to << "<nothing>";
		}
		return to;
	}

	void ExprResult::deepCopy(CloneEnv *env) {
		value.deepCopy(env);
	}

	ExprResult noReturn() {
		return ExprResult(false);
	}

	ExprResult common(ExprResult a, ExprResult b) {
		if (!a.any())
			return b;
		if (!b.any())
			return a;

		return common(a.type(), b.type());
	}

}
