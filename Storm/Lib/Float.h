#pragma once
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	class Str;

	class FloatType : public Type {
		STORM_CLASS;
	public:
		FloatType();

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::floatNr; }
		virtual Function *destructor() { return null; }

	protected:
		virtual bool loadAll();
	};

	// ToS for these types!
	STORM_PKG(core);
	Str *STORM_ENGINE_FN toS(EnginePtr e, Float v);

}
