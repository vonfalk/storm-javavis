#pragma once
#include "Gc.h"

namespace storm {

	class Engine;
	class Type;
	struct GcType;

	/**
	 * The Object root class. This is the base class of all objects allocated on the heap. As we're
	 * using a garbage collector, destructors of heap-allocated objects are not called promptly if
	 * at all. Destructors are called if one of the following conditions are satisfied:
	 *
	 * - A destructor has been *explicitly declared* in a class or any of its base- or derived classes.
	 * - A destructor has been *explicitly declared* in a member, which is stored by value.
	 */
	class Object {
		STORM_CLASS;
	public:
		// Default constructor.
		Object();

		// Default copy-constructor.
		Object(const Object &o);

		// Make sure destructors are virtual (we rely on this later on). Note: this special case is
		// excluded from the rules above. This destructor does not ensure that destructors are
		// actually called.
		virtual ~Object();

		// Get the engine somehow.
		Engine &engine() const;

		// Get our type somehow.
		inline Type *type() const {
			return Gc::allocType(this)->type;
		}

		// Used to allow the as<Foo> using our custom (faster) type-checking.
		bool isA(const Type *o) const;


		/**
		 * Allocation/deallocation.
		 *
		 * Deallocation is only here since C++ will automatically try to call operator delete in
		 * some circumstances, and is effectively a no-op.
		 */

		// Allocates memory for the type provided.
		static void *operator new(size_t size, Type *type);

		// Dummy matching deallocations.
		static void operator delete(void *mem, Type *type);

		// Special case for the first Type.
		static void *operator new(size_t size, Engine &e, GcType *type);
		static void operator delete(void *mem, Engine &e, GcType *type);
	};

}
