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

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::signedNr; }

		virtual Bool STORM_FN loadAll();
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

		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::signedNr; }

		virtual Bool STORM_FN loadAll();
	};

	// Create the Word type.
	Type *createWord(Str *name, Size size, GcType *type);


}
