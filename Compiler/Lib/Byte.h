#pragma once
#include "../Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * The Byte type.
	 */
	class ByteType : public Type {
		STORM_CLASS;
	public:
		ByteType(Str *name, GcType *type);

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::signedNr; }

		virtual Bool STORM_FN loadAll();
	};

	// Create the Byte type.
	Type *createByte(Str *name, Size size, GcType *type);

}
