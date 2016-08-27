#include "stdafx.h"
#include "Variable.h"
#include "Core/StrBuf.h"

namespace code {

	Variable::Variable() : id(-1) {}

	Variable::Variable(Nat id, Size size) : id(id), sz(size) {}

	wostream &operator <<(wostream &to, Variable v) {
		return to << L"Var" << v.id << L":" << v.sz;
	}

	StrBuf &operator <<(StrBuf &to, Variable v) {
		return to << L"Var" << v.id << L":" << v.sz;
	}


}
