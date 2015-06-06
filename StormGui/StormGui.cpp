#include "stdafx.h"
#include "StormGui.h"
#include "Shared/DllMain.h"

namespace stormgui {

	Test::Test() {}

	Str *Test::test() {
		return CREATE(Str, this, L"test");
	}

	ArrayP<Str> *Test::testArray() {
		Auto<ArrayP<Str>> arr = CREATE(ArrayP<Str>, this);
		arr->push(steal(CREATE(Str, this, L"A")));
		arr->push(steal(CREATE(Str, this, L"B")));
		return arr.ret();
	}
}
