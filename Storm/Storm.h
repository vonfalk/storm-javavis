#pragma once

#include "Code/Code.h"

// Mark the current package when exporting functions. Places all following
// functions in 'pkg'. For example: STORM_PKG(core)
#define STORM_PKG(pkg)

// Mark built-in functions with STORM_FN to get the right calling-convention
// and the automatic generation of function tables.
// For example: Str *STORM_FN foo();
#define STORM_FN CODECALL

// Mark the parent class as hidden from Storm. This is used in TObject.
// This more or less means that we copy all members of the parent class
// into this class, and treat them as separate entities in Storm.
// Example: class Foo : public STORM_HIDDEN(Bar) {};
#define STORM_HIDDEN(x) x

// Ignore the parent class in Storm. This is used to not have to expose the
// 'Printable' interface to Storm. Example: class Object : public STORM_IGNORE(Printable) {};
#define STORM_IGNORE(x) x

// Type code.
#define TYPE_CODE									\
	public:											\
	static Type *stormType(Engine &e);				\
	static Type *stormType(const Object *o);

#define TYPE_EXTRA_CODE									\
	public:												\
	template <class Z>									\
	static inline Type *stormType(const Auto<Z> &o) {	\
		return stormType(o.borrow());					\
	}													\
	template <class Z>									\
	static inline Type *stormType(const Par<Z> &o) {	\
		return stormType(o.borrow());					\
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
#define STORM_THREAD(name)										\
	struct name {												\
		static storm::Thread *thread(storm::Engine &e);			\
		static void force(storm::Engine &e, storm::Thread *to);	\
		static storm::DeclThread v;								\
	};

// Define the thread.
#define DEFINE_STORM_THREAD(name)							\
	storm::Thread *name::thread(storm::Engine &e) {			\
		return v.thread(e);									\
	}														\
	void name::force(storm::Engine &e, storm::Thread *to) { \
		v.force(e, to);										\
	}														\
	storm::DeclThread name::v;

namespace code {
	class Lock;
	class Sema;
}

namespace storm {
	class Engine;
	class Thread;

	using code::Size;
	using code::Offset;
	using code::Lock;
	using code::Sema;

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
