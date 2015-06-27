#pragma once
#include "Storm/Type.h"
#include "Storm/EnginePtr.h"
#include "Code/Code.h"

namespace storm {

	/**
	 * Long and Word implementations here:
	 */
	class LongType : public Type {
		STORM_CLASS;
	public:
		LongType();

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::signedNr; }
		virtual Function *destructor() { return null; }

	protected:
		virtual bool loadAll();
	};

	class WordType : public Type {
		STORM_CLASS;
	public:
		WordType();

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::unsignedNr; }
		virtual Function *destructor() { return null; }

	protected:
		virtual bool loadAll();
	};



	Str *STORM_ENGINE_FN toS(EnginePtr e, Long v);
	Str *STORM_ENGINE_FN toS(EnginePtr e, Word v);

}
