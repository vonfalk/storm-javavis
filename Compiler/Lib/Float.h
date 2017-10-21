#pragma once
#include "Compiler/Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * The Float type.
	 */
	class FloatType : public Type {
		STORM_CLASS;
	public:
		FloatType(Str *name, GcType *type);

		virtual BasicTypeInfo::Kind builtInType() const { return TypeKind::floatNr; }

	protected:
		virtual Bool STORM_FN loadAll();
	};

	// Create the float type.
	Type *createFloat(Str *name, Size size, GcType *type);


	/**
	 * The Double type.
	 */
	class DoubleType : public Type {
		STORM_CLASS;
	public:
		DoubleType(Str *name, GcType *type);

		virtual BasicTypeInfo::Kind builtInType() const { return TypeKind::floatNr; }

	protected:
		virtual Bool STORM_FN loadAll();
	};

	// Create the float type.
	Type *createDouble(Str *name, Size size, GcType *type);

}
