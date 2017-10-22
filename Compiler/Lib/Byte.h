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

	protected:
		virtual Bool STORM_FN loadAll();
		virtual code::TypeDesc *STORM_FN createTypeDesc() { return code::byteDesc(engine); }
	};

	// Create the Byte type.
	Type *createByte(Str *name, Size size, GcType *type);

}
