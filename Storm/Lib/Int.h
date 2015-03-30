#pragma once
#include "Storm/Type.h"
#include "Code/Code.h"

namespace storm {
	STORM_PKG(core.lang);

	class IntType : public Type {
		STORM_CLASS;
	public:
		IntType();

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::signedNr; }
		virtual Function *destructor() { return null; }

	protected:
		virtual void lazyLoad();
	};


	class NatType : public Type {
		STORM_CLASS;
	public:
		NatType();

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::unsignedNr; }
		virtual Function *destructor() { return null; }

	protected:
		virtual void lazyLoad();
	};


}
