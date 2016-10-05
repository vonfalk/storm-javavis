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

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::signedNr; }

		virtual Bool STORM_FN loadAll();
	};

	// Create the Int type.
	Type *createInt(Str *name, Size size, GcType *type);

}
