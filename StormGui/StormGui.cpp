#include "stdafx.h"
#include "StormGui.h"
#include "Shared/DllMain.h"

namespace stormgui {

	Test::Test() {}

	Str *Test::test() {
		return CREATE(Str, this, L"test");
	}

}
