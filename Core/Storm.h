#pragma once

/**
 * These are the global declarations Storm uses just about everywhere. These make the bridge between
 * Storm and C++ smaller, and makes it possible to use Storm-like types in C++.
 *
 * This file also defines several macros that are used as markers for the Storm preprocessor, so
 * that it knows what to expose to the Storm type system.
 *
 * Define STORM_COMPILER if we're being compiled as the compiler. Otherwise, we're assuming we're
 * being compiled to an external library of some kind.
 */

// Detect debug mode.
#include "Utils/Mode.h"


/**
 * Declare a super-class as hidden to Storm. This means that Storm will not know that the current
 * class inherits from any parent-class, even if it does from C++'s point of view.
 */
#define STORM_HIDDEN(parent) parent

/**
 * Common parts of STORM_CLASS and STORM_VALUE
 */
#define STORM_COMMON													\
	static Type *stormType(Engine &e);									\
	static Type *stormType(const Object *o);

/**
 * Mark classes and values exposed to storm:
 */
#define STORM_CLASS								\
	public:										\
	STORM_COMMON								\
	static inline void *operator new (size_t s, Type *t) { return storm::runtime::allocObject(s, t); } \
	static inline void operator delete (void *m, Type *t) {} \
	static inline void *operator new (size_t s, Engine &e) { return Object::operator new (s, stormType(e)); } \
	static inline void operator delete (void *m, Engine &e) {} \
	static inline void *operator new (size_t s, const Object *o) { return Object::operator new (s, stormType(o)); } \
	static inline void operator delete (void *m, const Object *o) {} \
	private:

#define STORM_VALUE								\
	public:										\
	STORM_COMMON								\
	private:

/**
 * Create classes in storm. Usage: CREATE(Type, engine, param1, param2, ...)
 */
#define CREATE(type, engine, ...)				\
	new (engine) type(__VA_ARGS__)

/**
 * Indicate which package to place things into (ignored by C++). (eg. STORM_PKG(foo.bar))
 */
#define STORM_PKG(pkg)

/**
 * Mark a function for export to Storm. (eg. void STORM_FN foo()).
 * Makes sure we're using the correct calling convention for Storm.
 *
 * If the function takes a EnginePtr as the first parameter (as a non-member), that parameter will
 * be filled in by the runtime to contain a Engine reference.
 */
#define STORM_FN __cdecl

/**
 * Mark a constructor exported to Storm.
 */
#define STORM_CTOR

/**
 * Mark unknown types for the preprocessor. Ignored by C++.
 */
#define UNKNOWN(kind)

/**
 * Declare a thread.
 */
#define STORM_THREAD(name)								\
	struct name {										\
		static storm::Thread *thread(storm::Engine &e); \
		static storm::DeclThread decl;					\
		static const Nat identifier;					\
	};

/**
 * Define a thread.
 */
#define STORM_DEFINE_THREAD(name)								\
	storm::Thread *name::thread(storm::Engine &e) {				\
		return decl.thread(e);									\
	}															\
	storm::DeclThread name::decl = { name::identifier, null };

/**
 * Define a thread, using a custom ThreadWait structure. 'fnPtr' is a pointer to a function like:
 * os::Thread foo(Engine &). That function is executed to create the thread.
 */
#define STORM_DEFINE_THREAD_WAIT(name, fnPtr)					\
	storm::Thread *name::thread(storm::Engine &e) {				\
		return decl.thread(e);									\
	}															\
	storm::DeclThread name::decl = { name::identifier, fnPtr };


#include "Utils/Utils.h"
#include "Utils/Platform.h"
#include "Types.h"
