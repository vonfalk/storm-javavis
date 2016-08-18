#pragma once
#include "Thread.h"
#include "Core/GcArray.h"
#include "Core/Array.h"
#include "Core/Set.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Implements a representation of the type-hierarchy in Storm where it is O(1) to check if a
	 * class is a subclass of another. Using Cohen's algorithm.
	 *
	 * TODO: Realize when a type died and remove it from any other TypeChain:s then!
	 */
	class TypeChain : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create a new chain owned by a type.
		STORM_CTOR TypeChain(Type *owner);

		// Set the super type to 'o'.
		void STORM_FN super(TypeChain *o);
		void STORM_FN super(Type *o);

		// Get the super type.
		Type *STORM_FN super() const;

		// Is this type derived from 'o'?
		Bool STORM_FN isA(const TypeChain *o) const;
		Bool STORM_FN isA(const Type *o) const;

		// Get the distance from another type, or -1 if not related.
		Int STORM_FN distance(const TypeChain *o) const;
		Int STORM_FN distance(const Type *o) const;

		// Get all currently known direct children. The result is _not_ ordered in any way.
		Array<Type *> *children() const;

		// Late initialization. Requires templates.
		void lateInit();

	private:
		// Our owner.
		Type *owner;

		// Chain of super types (eg. our supertype, that supertype, and so on).
		GcArray<TypeChain *> *chain;

		// Children. NOTE: This should probably be a weak set.
		Set<TypeChain *> *child;

		// Update our type chain.
		void updateSuper(const TypeChain *from);

		// Notify children about changes.
		void notify() const;

		// Clear our chain.
		void clearSuper();

		// Get the super TypeChain object.
		TypeChain *superChain() const;
	};

}
