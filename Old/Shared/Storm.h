#pragma once

#include "Utils/Utils.h"
#include "Utils/Platform.h"

// Are we being compiled as a DLL or as the compiler?
#if defined(STORM_LIB)
// We're the Shared lib.
#elif !defined(STORM_COMPILER)
#define STORM_DLL
#endif

// Mark the current package when exporting functions. Places all following
// functions in 'pkg'. For example: STORM_PKG(core)
#define STORM_PKG(pkg)

// Mark built-in functions with STORM_FN to get the right calling-convention
// and the automatic generation of function tables.
// For example: Str *STORM_FN foo();
#define STORM_FN CODECALL

// Mark built-in functions that should work as setters. No special treatment is done so far.
#define STORM_SETTER CODECALL

// Expose a data member to Storm. Example: STORM_VAR Auto<Str> foo;
#define STORM_VAR

// Mark built-in functions with STORM_FN_ENGINE to get the right calling-convention
// and the automatic generation of function tables. The difference from STORM_FN is
// that the first parameter (of type EnginePtr) will be generated automatically.
#define STORM_ENGINE_FN CODECALL

// Mark a built-in function to be executed on a specific thread. This is only
// applicable for non-member functions (no support for static members in the preprocessor).
// Example: void STORM_FN foo(Int a) ON(Compiler) {}
#define ON(thread)

// Mark the parent class as hidden from Storm. This is used in TObject.
// This more or less means that we copy all members of the parent class
// into this class, and treat them as separate entities in Storm.
// Example: class Foo : public STORM_HIDDEN(Bar) {};
#define STORM_HIDDEN(x) x

// Ignore the parent class in Storm. This is used to not have to expose the
// 'Printable' interface to Storm. Example: class Object : public STORM_IGNORE(Printable) {};
#define STORM_IGNORE(x) x

// Mark a nullable pointer.
#define MAYBE(x) x

// Define the name of the entry point in dynamic libraries based on if we're a debug build or not.
#if defined(FAST_DEBUG)
#define ENTRY_POINT_NAME stormFastDbgMain
#elif defined(DEBUG)
#define ENTRY_POINT_NAME stormDbgMain
#else
#define ENTRY_POINT_NAME stormMain
#endif

// Type code.
#define TYPE_CODE										\
	public:												\
	static storm::Type *stormType(storm::Engine &e);

#define TYPE_EXTRA_CODE												\
	public:															\
	static inline storm::Type *stormType(const storm::Object *o) {	\
		return stormType(storm::engine(o));							\
	}																\
	template <class Z>												\
	static inline storm::Type *stormType(const storm::Auto<Z> &o) {	\
		return stormType(o.borrow());								\
	}																\
	template <class Z>												\
	static inline storm::Type *stormType(const storm::Par<Z> &o) {	\
		return stormType(o.borrow());								\
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

// Mark an auto-cast constructor
#define STORM_CAST_CTOR

// Declare a thread.
#define STORM_THREAD(name)										\
	struct name {												\
		static storm::Thread *thread(storm::Engine &e);			\
		static storm::DeclThread decl;							\
	};

// Define the thread.
#define DEFINE_STORM_THREAD(name)							\
	storm::Thread *name::thread(storm::Engine &e) {			\
		return decl.thread(e);								\
	}														\
	storm::DeclThread name::decl = { null };

// Define the tread, using a custom ThreadWait structure. 'fnPtr' is a pointer to a function like:
// os::Thread foo(Engine &), and that function is executed to create the thread.
#define DEFINE_STORM_THREAD_WAIT(name, fnPtr)			\
	storm::Thread *name::thread(storm::Engine &e) {		\
		return decl.thread(e);							\
	}													\
	storm::DeclThread name::decl = { fnPtr };


namespace os {
	class Lock;
	class Sema;
	class Thread;
	class ThreadGroup;
}

namespace storm {
	class Engine;
	class Object;
	class Thread;
	class Type;

	template <class T>
	class Auto;

	template <class T>
	class Par;

	using os::Lock;
	using os::Sema;

#ifdef STORM_COMPILER
	class NamedThread;
#endif

	/**
	 * Class used when declaring named threads from C++.
	 */
	struct DeclThread {
		// Function used to create the ThreadWait (if any).
		// The address of this member is our unique identifier.
		typedef os::Thread (*CreateFn)(Engine &);
		CreateFn createFn;

		// Get the thread we are representing.
		Thread *thread(Engine &e) const;

		// Get our unique identifier.
		inline uintptr_t identifier() const {
			return (uintptr_t)&createFn;
		}

#ifdef STORM_COMPILER
		// When in the compiler, we can also get the NamedThread object associated to this
		// thread. It might be null during early startup or other inconvenient times.
		NamedThread *threadName(Engine &e) const;

		// Force the thread to something already created.
		void force(Engine &e, Thread *to);
#endif
	};


	// Get the Engine from an object (implemented in Object.cpp)
	Engine &engine(const Object *o);

	// Get the name of a type.
	String typeIdentifier(const Type *t);

	// Get a reference to the ThreadGroup for the current engine.
	os::ThreadGroup &threadGroup(Engine &e);
}

#include "Types.h"

#ifdef STORM_DLL
// For convenience.
#include "Object.h"
#include "TObject.h"
#include "Str.h"
#include "Array.h"
#include "Future.h"
#include "FnPtr.h"
#include "EnginePtr.h"
#include "Timing.h"
#endif