#pragma once
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * The Bool type.
	 */
	class BoolType : public Type {
		STORM_CLASS;
	public:
		BoolType(Str *name, GcType *type);

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::boolVal; }

	protected:
		virtual Bool STORM_FN loadAll();
	};

	// Create the float type.
	Type *createBool(Str *name, Size size, GcType *type);

}
