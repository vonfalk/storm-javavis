#pragma once
#include "Type.h"

namespace storm {

	class BoolType : public Type {
		STORM_CLASS;
	public:
		BoolType();

		virtual bool isBuiltIn() const { return true; }
		virtual code::Value destructor() const { return code::Value(); }
	};

}
