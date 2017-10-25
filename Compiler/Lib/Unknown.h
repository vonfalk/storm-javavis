#pragma once
#include "Compiler/Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Representations of types that are not properly known by Storm. The C++ preprocessor (in
	 * CppTypes/) will use these types to represent variables in value types that have no other
	 * proper representation in Storm. Storm needs to know about these variables to be able to
	 * generate accurate SrcDesc objects for function calls.
	 */


	/**
	 * Unknown variable.
	 */
	class UnknownType : public Type {
		STORM_CLASS;
	public:
		UnknownType(Str *name, Size size, GcType *type);

	protected:
		virtual Bool STORM_FN loadAll();
		virtual code::TypeDesc *STORM_FN createTypeDesc();
	};

	Type *createUnknownInt(Str *name, Size size, GcType *type);
	Type *createUnknownGc(Str *name, Size size, GcType *type);
	Type *createUnknownNoGc(Str *name, Size size, GcType *type);

}
