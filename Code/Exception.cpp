#include "stdafx.h"
#include "Exception.h"
#include "Value.h"

namespace code {

	String VariableUseError::what() const {
		return L"The variable #" + toS(Value(var)) + L" is not availiable in block" + toS(Value(block));
	}

}
