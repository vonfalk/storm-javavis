#pragma once
#include "Size.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Minimal description of a value passed to or from functions. This is needed as different
	 * calling conventions treat floats differently from other values.
	 */
	class ValType {
		STORM_VALUE;
	public:
		STORM_CTOR ValType(Size size, Bool isFloat);

		Size size;
		Bool isFloat;
	};

	ValType STORM_FN valVoid();
	ValType STORM_FN valPtr();

}
