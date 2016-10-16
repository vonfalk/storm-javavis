#pragma once
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Implements the Maybe<T> type in Storm. This type acts like a pointer, but is nullable. This
	 * type does not exist in C++, use MAYBE(Foo *) to mark pointers as nullable.
	 *
	 * TODO: Make it possible to use with any value.
	 */
	class MaybeType : public Type {
		STORM_CLASS;
	public:
		STORM_CTOR MaybeType(Str *name, Type *param);

		// Get the parameter.
		Value STORM_FN param() const;

		// Type.
		virtual BasicTypeInfo::Kind builtInType() const;

	private:
		// Contained type.
		Type *contained;
	};

	Bool STORM_FN isMaybe(Value v);
	Value STORM_FN unwrapMaybe(Value v);
	Value STORM_FN wrapMaybe(Value v);

	Type *createMaybe(Str *name, ValueArray *val);

}