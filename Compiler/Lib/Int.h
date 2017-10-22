#pragma once
#include "../Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * The Int type.
	 */
	class IntType : public Type {
		STORM_CLASS;
	public:
		IntType(Str *name, GcType *type);

	protected:
		virtual Bool STORM_FN loadAll();
		virtual code::TypeDesc *STORM_FN createTypeDesc() { return code::intDesc(engine); }
	};

	// Create the Int type.
	Type *createInt(Str *name, Size size, GcType *type);


	/**
	 * The Nat type.
	 */
	class NatType : public Type {
		STORM_CLASS;
	public:
		NatType(Str *name, GcType *type);

	protected:
		virtual Bool STORM_FN loadAll();
		virtual code::TypeDesc *STORM_FN createTypeDesc() { return code::intDesc(engine); }
	};

	// Create the Nat type.
	Type *createNat(Str *name, Size size, GcType *type);

}
