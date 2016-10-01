#pragma once
#include "Type.h"
#include "Core/GcType.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Declare the primitive types in here.
	 */
	Type *createBool(Str *name, Size size, GcType *type);
	Type *createByte(Str *name, Size size, GcType *type);
	Type *createInt(Str *name, Size size, GcType *type);
	Type *createNat(Str *name, Size size, GcType *type);
	Type *createLong(Str *name, Size size, GcType *type);
	Type *createWord(Str *name, Size size, GcType *type);

	/**
	 * Simple class for the primitive types for now. We want separate subclasses for each of them in
	 * the future.
	 */
	class PrimitiveType : public Type {
		STORM_CLASS;
	public:
		PrimitiveType(Str *name, Size size, GcType *type, BasicTypeInfo::Kind kind);

		virtual BasicTypeInfo::Kind builtInType() const;

	private:
		Int kind;
	};

}
