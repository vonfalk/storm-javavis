#pragma once
#include "Gc.h"

namespace storm {

	class Engine;
	class Type;
	class Str;
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
		 * Members common to all objects.
		 *
		 * Note: only TObjects usually have a equals and hash that involve the pointer-wise identity
		 * of the object. Make sure to note the compiler about any classes doing this, as it
		 * otherwise may misbehave when using hash maps.
		 *
		 * TODO: Re-think the equals-api.
		 */

		// Deep copy of all objects in here.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Convert to string.
		// TODO: Re-think using StrBuf!
		virtual Str *STORM_FN toS() const;

		// Equality check.
		virtual Bool STORM_FN equals(Object *o) const;

		// Hash function.
		virtual Nat STORM_FN hash() const;

		// Allow access.
		friend void *allocObject(size_t s, Type *t);
	};

	// Allocate an object (called internally from the STORM_CLASS macro).
	void *allocObject(size_t s, Type *t);

	// Output an object.
	wostream &operator <<(wostream &to, const Object *o);
	wostream &operator <<(wostream &to, const Object &o);
}
