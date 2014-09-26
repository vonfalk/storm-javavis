#pragma once

#include "Utils/Exception.h"

namespace code {

	// Thrown when an instruction contains invalid data for some reason.
	class InvalidValue : public Exception {
		String error;
	public:
		InvalidValue(const String &what) : error(what) {}
		virtual String what() const { return error; }
	};


}