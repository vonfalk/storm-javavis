#pragma once
#include "RootObject.h"

namespace storm {
	STORM_PKG(core);

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
	class Object : public STORM_HIDDEN(RootObject) {
		STORM_CLASS;
	public:
		// Default constructor.
		STORM_CTOR Object();

		/**
		 * Members common to all objects.
		 *
		 * TODO: Re-think the equals-api.
		 */

		// Deep copy of all objects in here.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Convert to string. TODO? Move toS(StrBuf *) to protected. CppTypes can be a friend to all types.
		virtual Str *STORM_FN toS() const;
		virtual void STORM_FN toS(StrBuf *to) const;

		// Equality check.
		virtual Bool STORM_FN equals(Object *o) const;

		// Hash function.
		virtual Nat STORM_FN hash() const;
	};

}

