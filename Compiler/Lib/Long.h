#pragma once
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * The Long type.
	 */
	class LongType : public Type {
		STORM_CLASS;
	public:
		LongType(Str *name, GcType *type);

	protected:
		virtual Bool STORM_FN loadAll();
		virtual code::TypeDesc *STORM_FN createTypeDesc() { return code::longDesc(engine); }
	};

	// Create the Long type.
	Type *createLong(Str *name, Size size, GcType *type);


	/**
	 * The Word type.
	 */
	class WordType : public Type {
		STORM_CLASS;
	public:
		WordType(Str *name, GcType *type);

	protected:
		virtual Bool STORM_FN loadAll();
		virtual code::TypeDesc *STORM_FN createTypeDesc() { return code::longDesc(engine); }
	};

	// Create the Word type.
	Type *createWord(Str *name, Size size, GcType *type);


}
