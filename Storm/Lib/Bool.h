#pragma once
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	class BoolType : public Type {
		STORM_CLASS;
	public:
		BoolType();

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::unsignedNr; }
		virtual Function *destructor() { return null; }
	};

}
