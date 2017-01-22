#pragma once

// Forward...
#include "OS/FnParams.h"

namespace code {

	// Get the size of the FnParams class (for machine code generation).
	Size fnParamsSize();

	// Get the size of a parameter passed to FnParams (for machine code generation).
	Size fnParamSize();

}
