#pragma once
#include "Thread.h"
#include "SrcPos.h"
#include "Value.h"

namespace storm {
	STORM_PKG(core.lang);

	class Name;

	class Scope {
		STORM_VALUE;
	public:
		// Look up a value. Throws on error.
		Value value(Name *name, SrcPos pos) const;
	};

}
