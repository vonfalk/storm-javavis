#include "stdafx.h"
#include "ValType.h"

namespace code {

	ValType::ValType(Size size, Bool isFloat) : size(size), isFloat(isFloat) {}

	ValType valVoid() {
		return ValType(Size(), false);
	}

	ValType valPtr() {
		return ValType(Size::sPtr, false);
	}

}
