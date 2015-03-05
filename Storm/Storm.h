#pragma once

#include "Code/Code.h"

// Mark the current package when exporting functions. Places all following
// functions in 'pkg'. For example: STORM_PKG(core)
#define STORM_PKG(pkg)

// Mark built-in functions with STORM_FN to get the right calling-convention
// and the automatic generation of function tables.
// For example: Str *STORM_FN foo();
#define STORM_FN CODECALL

// Type code.
#define TYPE_CODE									\
	public:											\
	static Type *type(Engine &e);					\
	static Type *type(const Object *o);

#define TYPE_EXTRA_CODE								\
	public:											\
	template <class Z>								\
	static inline Type *type(const Auto<Z> &o) {	\
		return type(o.borrow());					\
	}												\
	template <class Z>								\
	static inline Type *type(const Par<Z> &o) {		\
		return type(o.borrow());					\
	}

// Mark a value-type.
#define STORM_VALUE \
	TYPE_CODE		\
	TYPE_EXTRA_CODE \
private:

// Mark a class(object)-type.
#define STORM_CLASS								\
	TYPE_CODE									\
	TYPE_EXTRA_CODE								\
	static void *cppVTable();					\
private:

// Mark a constructor.
#define STORM_CTOR

// Declare a thread.
#define STORM_THREAD(name)						\
	extern storm::DeclThread name

// Define the thread.
#define DEFINE_STORM_THREAD(name)				\
	storm::DeclThread name

namespace storm {
	class Engine;
	class Thread;

	using code::Size;
	using code::Offset;

	/**
	 * Class used when declaring named threads from C++.
	 */
	struct DeclThread {
		// The address of this member is our unique identifier.
		nat dummy;

		// Get the thread we are representing.
		Thread *thread(Engine &e) const;

		// Force the thread to something already created.
		void force(Engine &e, Thread *to);
	};
}
