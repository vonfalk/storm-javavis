#include "stdafx.h"
#include "ExprResult.h"
#include "Shared/Str.h"
#include "Shared/EnginePtr.h"

namespace storm {

	ExprResult::ExprResult() : returns(true) {}

	ExprResult::ExprResult(Value value) : type(value), returns(true) {}

	ExprResult::ExprResult(bool any) : returns(any) {}

	Value ExprResult::result() const {
		if (returns)
			return type;
		else
			return Value();
	}

	Bool ExprResult::any() const {
		return returns;
	}

	Bool ExprResult::operator ==(const ExprResult &o) const {
		return returns == o.returns && type == o.type;
	}

	Bool ExprResult::operator !=(const ExprResult &o) const {
		return !(*this == o);
	}

	wostream &operator <<(wostream &to, const ExprResult &r) {
		if (r.any()) {
			to << r.result();
		} else {
			to << "<never returns>";
		}
		return to;
	}

	void ExprResult::deepCopy(Par<CloneEnv> env) {
		type.deepCopy(env);
	}

	ExprResult noReturn() {
		return ExprResult(false);
	}

	Str *toS(EnginePtr e, ExprResult from) {
		return CREATE(Str, e.v, ::toS(from));
	}


}
