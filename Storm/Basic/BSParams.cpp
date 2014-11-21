#include "stdafx.h"
#include "BSParams.h"

namespace storm {

	bs::Params::Params() {}

	void bs::Params::add(Auto<TypeName> type) {
		params.push_back(type);
	}

	void bs::Params::output(wostream &to) const {
		to << L"(";
		join(to, params, L", ");
		to << L")";
	}
}
