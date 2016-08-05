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
	static inline const Handle &stormHandle(Engine &e) { return storm::runtime::typeHandle(stormType(e)); }

#define STORM_TYPE_DECL							\
	static const Nat stormTypeId;				\
	static Type *stormType(Engine &e);			\
	static Type *stormType(const Object *o);

/**
 * Common parts for all heap-allocated objects.
 */
#define STORM_OBJ_COMMON												\
	STORM_COMMON														\
	static inline void *operator new (size_t s, Type *t) { return storm::runtime::allocObject(s, t); } \
	static inline void operator delete (void *m, Type *t) {}			\
	static inline void *operator new (size_t s, Engine &e) { return Object::operator new (s, stormType(e)); } \
	static inline void operator delete (void *m, Engine &e) {}			\
	static inline void *operator new (size_t s, const Object *o) { return Object::operator new (s, stormType(o)); } \
	static inline void operator delete (void *m, const Object *o) {}

/**
 * Special case for storm::Object.
 */
#define STORM_OBJ_CLASS							\
	public:										\
	STORM_TYPE_DECL								\
	STORM_OBJ_COMMON							\
	private:

/**
 * Mark classes that are not exported and provide their own 'stormType' implementation. Not
 * automatically exposed to Storm.
 */
#define STORM_SPECIAL													\
	public:																\
	STORM_OBJ_COMMON													\
	static Type *stormType(const Object *o) { return stormType(o->engine()); } \
	using Object::toS;													\
	private:

/**
 * Mark classes and values exposed to storm:
 */

// Mark a class or actor.
#define STORM_CLASS								\
	public:										\
	STORM_TYPE_DECL								\
	STORM_OBJ_COMMON							\
	using Object::toS;							\
	private:

// Mark a value.
#define STORM_VALUE								\
	public:										\
	STORM_TYPE_DECL								\
	STORM_COMMON								\
	private:

/**
 * Make a template type id. This will only make Storm aware of the template, you still have to
 * implement the template class itself in the compiler somewhere. 'generator' is a function like
 * MAYBE(Type) *foo(ValueArray *params). This can only be done in the compiler at the moment, since
 * no other libraries can create types. TODO: allow 'generator' to be any function in Storm as well.
 */
#define STORM_TEMPLATE(name, generator)			\
	extern const Nat name ## Id;

/**
 * Create classes in storm. Usage: CREATE(Type, engine, param1, param2, ...)
 */
#define CREATE(type, engine, ...)				\
	new (engine) type(__VA_ARGS__)

/**
 * Declare a pointer that may be null. Ignored by C++.
 */
#define MAYBE(x) x

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
#define STORM_CTOR explicit

/**
 * Mark a constructor exported to Storm that can be used for automatic casts.
 */
#define STORM_CAST_CTOR

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
#include "Runtime.h"
