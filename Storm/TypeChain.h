#pragma once

namespace storm {

	class Type;

	/**
	 * Defines the inheritance hierarchy for types along with fast
	 * lookups for super-types. Using Cohen's algorithm.
	 */
	class TypeChain : NoCopy {
	public:
		// Initialize to a separate type.
		TypeChain(Type *owner);

		// Dtor.
		~TypeChain();

		// Set the super type to 'o'.
		void super(Type *o);

		// Get the super type to 'o'.
		Type *super() const;

		// Is this derived from 'o'?
		bool isA(TypeChain *o) const;
		bool isA(Type *o) const;

		// Get all currently known direct children. Note that the result is not
		// ordered in any way at all, even though it is a vector.
		vector<Type *> children() const;

	private:
		// The chain of super types. Always ends with 'this'.
		TypeChain **chain;

		// Number of types
		nat count;

		// We need to update child types when we're updated.
		typedef set<TypeChain *> ChildSet;
		ChildSet child;

		// Owner.
		Type *owner;

		// Update our type chain.
		void updateSuper(const TypeChain &from);

		// Clear our type chain.
		void clearSuper();

		// Notify children about changes.
		void notify() const;

		// Get the super TypeChain object.
		TypeChain *superChain() const;
	};

}
