#pragma once
#include "Object.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Function pointers for storm and C++.
	 *
	 * A function pointer here may also contain a this-pointer as well, so that member functions
	 * appear like a regular free function without the first this-parameter.
	 *
	 * Function pointers have slightly different calls semantics compared to regular function calls
	 * in Storm; for function pointers, it is determined entirely at runtime if the parameters are
	 * copied or not. This is because we do not have the same amount of information when dealing
	 * with function pointers.
	 *
	 * Function pointers to members of values are not yet supported.
	 */

	/**
	 * Target of a function call. Implemented as an abstract class here, so we do not need to expose
	 * Ref and RefSource to everyone.
	 *
	 * These are too low-level to be exposed to Storm.
	 */
	class FnTarget {
	public:
		// Clone this target to somewhere else.
		virtual void cloneTo(void *to, size_t size) const = 0;

		// Get the pointer we're pointing to.
		virtual const void *ptr() const = 0;

		// Add some kind of label to a StrBuf for the string representation.
		virtual void toS(StrBuf *to) const = 0;
	};

	/**
	 * Target for raw function pointers.
	 */
	class RawFnTarget {
	public:
		RawFnTarget(const void *ptr);

		virtual void cloneTo(void *to, size_t size) const;
		virtual const void *ptr() const;
		virtual void toS(StrBuf *to) const;
	private:
		const void *data;
	};

	/**
	 * Base class for a function pointer.
	 *
	 * NOTE: Support for Ref and RefSources are implemented as a subclass in Compiler/
	 */
	class FnBase : public Object {
		STORM_CLASS;
	public:
		// Create from C++. Assumes we're not creating a function pointer to a function marked
		// ON(Thread), as C++ does not see those. Also assumes that if a 'this' pointer is passed,
		// we're calling a member function.
		FnBase(const void *fn, RootObject *thisPtr);

		// Create with a generic target and all information given.
		FnBase(const Target &target, RootObject *thisPtr, Bool member, Thread *thread);

		// Copy.
		STORM_CTOR FnBase(FnBase *o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	public:
		// Are we calling a member function?
		Bool callMember;

		// This pointer.
		UNKNOWN(PTR_GC) RootObject *thisPtr;

		// Thread to call on.
		Thread *thread;

		// Storage for target. Two words are enough for now.
		enum { targetSize = 2 };
		size_t target0;
		size_t target1;

		// Get target.
		inline FnTarget *target() const { return (FnTarget *)&target0; }
	};


	// Declare the template to Storm.
	// STORM_TEMPLATE(Fn, createFn);

}
