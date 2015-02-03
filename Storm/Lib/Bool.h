#pragma once
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	class BoolType : public Type {
		STORM_CLASS;
	public:
		BoolType();

		virtual bool isBuiltIn() const { return true; }
		virtual Function *destructor() { return null; }
	};

}
