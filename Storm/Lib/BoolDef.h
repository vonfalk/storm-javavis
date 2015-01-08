#pragma once
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	class BoolType : public Type {
		STORM_CLASS;
	public:
		BoolType();

		virtual bool isBuiltIn() const { return true; }
		virtual code::Value destructor() const { return code::Value(); }
	};

}
