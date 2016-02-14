#pragma once
#include "Storm/Type.h"
#include "Storm/EnginePtr.h"
#include "Code/Code.h"

namespace storm {
	STORM_PKG(core.lang);

	class Str;

	class IntType : public Type {
		STORM_CLASS;
	public:
		IntType();

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::signedNr; }
		virtual Function *destructor() { return null; }

		virtual Bool STORM_FN loadAll();
	};

	class NatType : public Type {
		STORM_CLASS;
	public:
		NatType();

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::unsignedNr; }
		virtual Function *destructor() { return null; }

		virtual Bool STORM_FN loadAll();
	};

	class ByteType : public Type {
		STORM_CLASS;
	public:
		ByteType();

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::unsignedNr; }
		virtual Function *destructor() { return null; }

		virtual Bool STORM_FN loadAll();
	};

	// ToS for these types!
	STORM_PKG(core);
	Str *STORM_ENGINE_FN toS(EnginePtr e, Int v);
	Str *STORM_ENGINE_FN toS(EnginePtr e, Nat v);
	Str *STORM_ENGINE_FN toS(EnginePtr e, Byte v);

}
