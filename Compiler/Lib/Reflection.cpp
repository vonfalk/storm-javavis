#include "stdafx.h"
#include "Reflection.h"

namespace storm {
	namespace reflect {

		Type *typeOf(Object *o) {
			return runtime::typeOf(o);
		}

		Type *typeOf(TObject *o) {
			return runtime::typeOf(o);
		}

	}
}
