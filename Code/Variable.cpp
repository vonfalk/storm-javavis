#include "stdafx.h"
#include "Variable.h"
#include "Core/StrBuf.h"

namespace code {

	Variable::Variable(Nat id, Size size) : id(id), sz(size) {}

	wostream &operator <<(wostream &to, Variable v) {
		return to << v.id << L"(" << v.sz << L")";
	}

	StrBuf &operator <<(StrBuf &to, Variable v) {
		return to << v.id << L"(" << v.sz << L")";
	}


}
