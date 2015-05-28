#pragma once
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	// Add the maybe template.
	void addMaybeTemplate(Par<Package> to);

	// Get the maybe type.
	Type *maybeType(Engine &e, const Value &v);


	/**
	 * Implements the Maybe<T> type in Storm. This type acts like a pointer, but is nullable.
	 * This type does not exist in C++, use the MAYBE() macro to mark pointers as nullable.
	 *
	 * TODO: Copy the parent's thread value as well?
	 */
	class MaybeType : public Type {
		STORM_CLASS;
	public:
		MaybeType(const Value &param);

		// Parameter type.
		const Value param;

		// Lazy loading.
		virtual bool loadAll();

	private:
		// Create copy ctors.
		Named *createCopy(Par<NamePart> part);

		// Create assignment operators.
		Named *createAssign(Par<NamePart> part);
	};

	// Check if a value represents a Maybe type.
	Bool STORM_FN isMaybe(Value v);

	// Get the value of a maybe type. If 'v' is not a maybe, 'v' is returned without modification.
	Value STORM_FN unwrapMaybe(Value v);

	// Wrap a type in maybe.
	Value STORM_FN wrapMaybe(Value v);

}
