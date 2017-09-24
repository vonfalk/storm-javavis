#include "stdafx.h"
#include "ValType.h"

namespace code {

	ValType::ValType(Size size, Bool isFloat) : size(size), isFloat(isFloat) {}

	ValType valVoid() {
		return ValType(Size(), false);
	}

	ValType valByte() {
		return ValType(Size::sByte, false);
	}

	ValType valInt() {
		return ValType(Size::sInt, false);
	}

	ValType valLong() {
		return ValType(Size::sLong, false);
	}

	ValType valPtr() {
		return ValType(Size::sPtr, false);
	}

}
