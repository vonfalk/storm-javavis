#pragma once
#include "Type.h"
#include "Code/Code.h"

namespace storm {
	STORM_PKG(core.lang);

	class IntType : public Type {
		STORM_CLASS;
	public:
		IntType();

		virtual bool isBuiltIn() const { return true; }
		virtual Function *destructor() { return null; }

	protected:
		virtual void lazyLoad();
	};


	class NatType : public Type {
		STORM_CLASS;
	public:
		NatType();

		virtual bool isBuiltIn() const { return true; }
		virtual Function *destructor() { return null; }

	protected:
		virtual void lazyLoad();
	};


}
