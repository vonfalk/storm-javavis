#include "stdafx.h"
#include "BuiltIn.h"
#include <stdarg.h>

// Below are auto-generated includes.
// BEGIN INCLUDES
#include "Str.h"
// END INCLUDES

namespace storm {

	/**
	 * Create a vector from a argument list.
	 */
	vector<Name> list(nat count, ...) {
		va_list l;
		va_start(l, count);

		vector<Name> result;
		for (nat i = 0; i < count; i++)
			result.push_back(va_arg(l, Name));

		va_end(l);
		return result;
	}

	/**
	 * Test function (in the meantime...)
	 */
	Str *testFn(Str *v) {
		PLN("TestFn: " << v->v);
		return v;
	}

	/**
	 * Everything between BEGIN LIST and END LIST is auto-generated.
	 */
	const BuiltInFunction *builtInFunctions() {
		static BuiltInFunction fns[] = {
			// BEGIN LIST
			{ Name(L"core"), Name(L"Str"), L"test", list(1, Name(L"Str")), &testFn },
			// END LIST
			{ Name(), Name(), L"", list(0), null },
		};
		return fns;
	}

}
