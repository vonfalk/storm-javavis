#include "stdafx.h"
#include "Var.h"
#include "Core/StrBuf.h"

namespace code {

	Var::Var() : id(-1) {}

	Var::Var(Nat id, Size size) : id(id), sz(size) {}

	wostream &operator <<(wostream &to, Var v) {
		return to << L"Var" << v.id << L":" << v.sz;
	}

	StrBuf &operator <<(StrBuf &to, Var v) {
		return to << L"Var" << v.id << L":" << v.sz;
	}


}
