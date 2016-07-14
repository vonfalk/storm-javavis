#pragma once

/**
 * Storm supports a few garbage collectors. Choose one here:
 *
 * TODO: Make it possible to choose as a parameter to mymake.
 *
 * MPS is the fastest and most flexible, but requires all programs using it to make the source code
 * available (or to acquire another license for the MPS).
 *
 * TODO: Add support for boehm GC as well: http://hboehm.info/gc/ (very liberal license, not as powerful).
 */

// C-mode detection of debug mode.
#include "Utils/Mode.h"

#define STORM_GC_MPS
// #define STORM_GC_BOEHM

// When compiling in C-mode, we do not want to include these:
#ifdef __cplusplus

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
	static inline void *operator new (size_t s, Type *t) { return storm::allocObject(s, t); } \
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


#include "Utils/Utils.h"
#include "Utils/Platform.h"
#include "Code/Code.h"
#include "Types.h"

#endif
