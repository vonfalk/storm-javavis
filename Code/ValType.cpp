#include "stdafx.h"
#include "ValType.h"

namespace code {

	ValType::ValType(Size size, Bool isFloat) : size(size), isFloat(isFloat) {}

	ValType valVoid() {
		return ValType(Size(), false);
	}

	ValType valInt() {
		return ValType(Size::sInt, false);
	}

	ValType valPtr() {
		return ValType(Size::sPtr, false);
	}

}
