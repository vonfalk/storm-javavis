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

	protected:
		virtual Bool STORM_FN loadAll();
		virtual code::TypeDesc *STORM_FN createTypeDesc() { return code::byteDesc(engine); }
	};

	// Create the float type.
	Type *createBool(Str *name, Size size, GcType *type);

}
