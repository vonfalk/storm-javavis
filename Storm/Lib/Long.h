#pragma once
#include "Storm/Type.h"
#include "Storm/EnginePtr.h"
#include "Code/Code.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Long and Word implementations here:
	 */
	class LongType : public Type {
		STORM_CLASS;
	public:
		LongType();

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::signedNr; }
		virtual Function *destructor() { return null; }

		virtual Bool STORM_FN loadAll();
	};

	class WordType : public Type {
		STORM_CLASS;
	public:
		WordType();

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::unsignedNr; }
		virtual Function *destructor() { return null; }

		virtual Bool STORM_FN loadAll();
	};


	// toS!
	STORM_PKG(core);
	Str *STORM_ENGINE_FN toS(EnginePtr e, Long v);
	Str *STORM_ENGINE_FN toS(EnginePtr e, Word v);

}
