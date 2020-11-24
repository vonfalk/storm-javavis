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

// Detect debug mode and platform.
#include "Utils/Mode.h"
#include "Utils/Platform.h"


/**
 * Declare a super-class as hidden to Storm. This means that Storm will not know that the current
 * class inherits from any parent-class, even if it does from C++'s point of view.
 *
 * Can also be used to make enums not being exported to Storm.
 */
#define STORM_HIDDEN(parent) parent

/**
 * Convert a thing to a Str using a string buffer. Assumes that StrBuf is properly included when
 * used.
 *
 * Example:
 * TO_S(engine(), foo << L", " << bar);
 */
#define TO_S(engine, concat) (*new (engine) storm::StrBuf() << concat).toS()

/**
 * Some implementations (most notably GCC) require a 'key method' to be declared as the first member
 * in a class, that is defined in the CppTypes.cpp-file in order to make sure that vtables are
 * generated properly for abstract classes.
 *
 * See "Vague Linkage" in the GCC manual for details: https://gcc.gnu.org/onlinedocs/gcc/Vague-Linkage.html
 */
#if defined(GCC)
#define STORM_NEED_KEY_FUNCTION
#endif

#ifdef STORM_NEED_KEY_FUNCTION
#define STORM_KEY_FUNCTION_DECL virtual void storm_key_function();
#define STORM_KEY_FUNCTION_IMPL(x) void x::storm_key_function() {}
#else
#define STORM_KEY_FUNCTION_DECL
#define STORM_KEY_FUNCTION_IMPL(x)
#endif


/**
 * Common parts of STORM_CLASS and STORM_VALUE
 */
#define STORM_COMMON													\
	friend struct storm::CppMeta;										\
	static inline const storm::Handle &stormHandle(storm::Engine &e) {	\
		return storm::runtime::typeHandle(stormType(e));				\
	}																	\
	static inline void *operator new (size_t s, storm::Place mem) {		\
		return mem.ptr;													\
	}																	\
	static inline void operator delete (void *, storm::Place) {}

#define STORM_TYPE_DECL									\
	static const storm::Nat stormTypeId;				\
	static storm::Type *stormType(storm::Engine &e);			\
	static storm::Type *stormType(const storm::RootObject *o);

/**
 * Common parts for all heap-allocated objects.
 */
#define STORM_OBJ_COMMON												\
	STORM_COMMON														\
	static inline void *operator new (size_t s, storm::Engine &e) {		\
		return storm::runtime::allocObject(s, stormType(e));			\
	}																	\
	static inline void operator delete (void *m, storm::Engine &e) {}	\
	static inline void *operator new (size_t s, const storm::RootObject *o) { \
		return storm::runtime::allocObject(s, stormType(o));			\
	}																	\
	static inline void operator delete (void *m, const storm::RootObject *o) {} \
	static inline void operator delete (void *m, size_t x) {}


/**
 * Special case for storm::RootObject.
 */
#define STORM_ROOT_CLASS						\
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
	static storm::Type *stormType(const storm::RootObject *o) { return stormType(o->engine()); } \
	using storm::RootObject::toS;										\
	private:

/**
 * Mark classes and values exposed to storm:
 */

// Mark a class or actor.
#define STORM_CLASS								\
	public:										\
	STORM_TYPE_DECL								\
	STORM_OBJ_COMMON							\
	using storm::RootObject::toS;				\
	private:

// Mark an abstract class or actor. Same as 'STORM_CLASS', but sometimes need to introduce an
// additional 'key method' for proper vtable generation in C++ (see above for details).
#define STORM_ABSTRACT_CLASS					\
	public:										\
	STORM_KEY_FUNCTION_DECL						\
	STORM_TYPE_DECL								\
	STORM_OBJ_COMMON							\
	using storm::RootObject::toS;				\
	private:

// Mark a value.
#define STORM_VALUE								\
	public:										\
	STORM_TYPE_DECL								\
	STORM_COMMON								\
	private:


// Note that we use the 'throwMe' function in exceptions as a key method. Therefore, we don't need
// to do anything special for abstract exception classes.
#define STORM_EXCEPTION_FN_DECL virtual void CODECALL throwMe();
#define STORM_EXCEPTION_FN_IMPL(x) void x::throwMe() { throw this; }

// Mark a class that inherits from Exception and that can be thrown.
// Exceptions are allowed to be abstract, so no extra modifier is needed for that.
#define STORM_EXCEPTION							\
	public:										\
	STORM_EXCEPTION_FN_DECL						\
	STORM_TYPE_DECL								\
	STORM_OBJ_COMMON							\
	using storm::RootObject::toS;				\
	using storm::Exception::message;			\
	private:

// Mark the root exception class.
#define STORM_EXCEPTION_BASE					\
	public:										\
	STORM_EXCEPTION_FN_DECL						\
	STORM_TYPE_DECL								\
	STORM_OBJ_COMMON							\
	using storm::RootObject::toS;				\
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
 * Make a primitive type-id. This is used to declare the primitive types known to Storm in Handle.h,
 * so that StormInfo<T> works on them too.
 */
#define STORM_PRIMITIVE(name, generator)								\
	extern const Nat name ## Id;										\
	template <>															\
	struct StormInfo<name> {											\
		static Nat id() {												\
			return name ## Id;											\
		}																\
		static const storm::Handle &handle(storm::Engine &e) {			\
			return runtime::typeHandle(runtime::cppType(e, name ## Id)); \
		}																\
		static storm::Type *type(storm::Engine &e) {					\
			return runtime::cppType(e, id());							\
		}																\
	}

/**
 * Declare a primitive to be used for an unknown variable (eg. UNKNOWN(PTR_GC))
 */
#define STORM_UNKNOWN_PRIMITIVE(name, generator)

/**
 * Create classes in storm. Usage: CREATE(Type, engine, param1, param2, ...)
 */
#define CREATE(type, engine, ...)				\
	new (engine) type(__VA_ARGS__)

/**
 * Declare a pointer that may be null. Ignored by C++. Variadic since we want to support MAYBE(Map<Foo, Bar>) properly.
 */
#define MAYBE(...) __VA_ARGS__

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
#define STORM_FN CODECALL

/**
 * Mark a function as a setter function. These functions will be callable as 'foo = x' => 'foo(x)'.
 */
#define STORM_ASSIGN STORM_FN

/**
 * Mark a constructor exported to Storm.
 */
#define STORM_CTOR explicit

/**
 * Mark a constructor exported to Storm that can be used for automatic casts.
 */
#define STORM_CAST_CTOR

/**
 * Mark a function needing to be executed on a specific thread. Can be used on global functions and
 * in values. Example: void STORM_FN foo() ON(Compiler);
 */
#define ON(x)

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
		static const storm::Nat identifier;				\
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

/**
 * Declare an alternate name for something in Storm. This can be used when one wants to use a
 * keyword in C++ as an identifier for something in Storm. Declaring it as STORM_NAME(a, b) uses 'a'
 * as an identifier in C++ and 'b' as an identifier in Storm.
 */
#define STORM_NAME(x, y) x

/**
 * Allow using abstract functions with the 'abstract' keyword.
 *
 * This allows us to re-define it in order to instantiate abstract classes to make them extensible
 * from Storm.
 */
#define ABSTRACT = 0

/**
 * The name of entry points for this build. The debug and release builds look slightly different
 * memory-wise here and there, so the entry points of the shared libraries have different name to
 * avoid confusion and strange crashes.
 */
#ifdef DEBUG
#ifndef FAST_DEBUG
#define SHARED_LIB_ENTRY storm_start_slow
#else
#define SHARED_LIB_ENTRY storm_start_debug
#endif
#else
#define SHARED_LIB_ENTRY storm_start
#endif


#include "Utils/Utils.h"
#include "Types.h"
#include "Runtime.h"
#include "Compare.h"

/**
 * Declare a string literal in Storm. Expands to L"" or u"" depending on what is appropriate for the
 * current system.
 *
 * LITERAL_S(x) produces whatever expression is passed as x. For example, LITERAL_S(1 + 2) yields "1 + 2"
 */
#ifdef WINDOWS
#define S_(x) L ## x
#define S(x) S_(x)
#define LITERAL__S(x) L ## #x
#define LITERAL_S(x) LITERAL__S(x)
#else
#define S_(x) u ## x
#define S(x) S_(x)
#define LITERAL__S(x) u ## #x
#define LITERAL_S(x) LITERAL__S(x)
#endif
