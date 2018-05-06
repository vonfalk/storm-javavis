#pragma once
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Implements the Maybe<T> type in Storm. This type acts like a pointer, but is nullable. This
	 * type does not exist in C++, use MAYBE(Foo *) to mark pointers as nullable. Nullable value
	 * types are not supported in C++.
	 */
	class MaybeType : public Type {
		STORM_CLASS;
	public:
		MaybeType(Str *name, Type *param, TypeFlags flags, Size size, GcType *gcType);

		// Get the parameter.
		Value STORM_FN param() const;

	protected:
		// Contained type.
		Type *contained;
	};

	/**
	 * Implementation of Maybe<T> for classes.
	 *
	 * This class essentially represents the underlying pointer, and only serves as a marker that it
	 * might be null.
	 */
	class MaybeClassType : public MaybeType {
		STORM_CLASS;
	public:
		STORM_CTOR MaybeClassType(Str *name, Type *param);

	protected:
		// Lazy-loading.
		virtual Bool STORM_FN loadAll();
		virtual code::TypeDesc *STORM_FN createTypeDesc();

	private:
		// Create copy ctors.
		Named *CODECALL createCopy(Str *name, SimplePart *part);

		// Create assignment operators.
		Named *CODECALL createAssign(Str *name, SimplePart *part);
	};


	/**
	 * Implementation of Maybe<T> for values. Works like MaybeType, but since the implementation
	 * differs quite a lot, the logic for values was put separately from classes and actors.
	 *
	 * This class has the same layout as the original value, but an additional boolean is appended
	 * to the end to indicate whether or not the value stored is actually valid.
	 */
	class MaybeValueType : public MaybeType {
		STORM_CLASS;
	public:
		STORM_CTOR MaybeValueType(Str *name, Type *param);

	protected:
		// Lazy-loading.
		virtual Bool STORM_FN loadAll();
		virtual code::TypeDesc *STORM_FN createTypeDesc();

	private:
		// Create copy ctors.
		Named *CODECALL createCopy(Str *name, SimplePart *part);

		// Create assignment operators.
		Named *CODECALL createAssign(Str *name, SimplePart *part);

		// Offset of the boolean flag. Corresponds to 'any', ie. 1 if value present, otherwise 0.
		Offset boolOffset() const { return Offset(contained->size()); }

		// Other misc. code generation helpers.
		void CODECALL initMaybe(InlineParams p);
		void CODECALL copyMaybe(InlineParams p);
		void CODECALL emptyMaybe(InlineParams p);
		void CODECALL anyMaybe(InlineParams p);
	};


	Bool STORM_FN isMaybe(Value v);
	Value STORM_FN unwrapMaybe(Value v);
	Value STORM_FN wrapMaybe(Value v);

	Type *createMaybe(Str *name, ValueArray *val);

}
