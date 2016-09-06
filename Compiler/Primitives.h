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
	 * Dummy class (for now) causing this file to be included in CppTypes.cpp. We want to make
	 * separate types for all primitive types later.
	 */
	class Dummy : public Type {
		STORM_CLASS;
	public:
	};

}
