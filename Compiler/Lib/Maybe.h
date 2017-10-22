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

	protected:
		// Lazy-loading.
		virtual Bool STORM_FN loadAll();
		virtual code::TypeDesc *STORM_FN createTypeDesc() { return code::ptrDesc(engine); }

	private:
		// Contained type.
		Type *contained;

		// Create copy ctors.
		Named *CODECALL createCopy(Str *name, SimplePart *part);

		// Create assignment operators.
		Named *CODECALL createAssign(Str *name, SimplePart *part);
	};

	Bool STORM_FN isMaybe(Value v);
	Value STORM_FN unwrapMaybe(Value v);
	Value STORM_FN wrapMaybe(Value v);

	Type *createMaybe(Str *name, ValueArray *val);

}
