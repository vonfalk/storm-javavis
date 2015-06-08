#include "stdafx.h"
#include "FnParams.h"

namespace code {

	Size fnParamsSize() {
		Size s = Size::sPtr;
		s += Size::sNat;
		s += Size::sNat;
		assert(s.current() == sizeof(os::FnParams), L"Please update the size here.");
		return s;
	}

	Size fnParamSize() {
		Size s = Size::sPtr * 3;
		s += Size::sNat;
		assert(s.current() == sizeof(os::FnParams::Param), L"Please update the size here.");
		return s;
	}

}
