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
	static inline Type *stormType(Object *o) { return stormType(o->engine()); } \

/**
 * Mark classes and values exposed to storm:
 */
#define STORM_CLASS								\
	public:										\
	STORM_COMMON								\
	static inline void *operator new (size_t s, Engine &e) { return Object::operator new (s, stormType(e)); } \
	static inline void operator delete (void *m, Engine &e) {} \
	static inline void *operator new (size_t s, Object *o) { return Object::operator new (s, stormType(o)); } \
	static inline void operator delete (void *m, Object *o) {} \
	private:

#define STORM_VALUE								\
	public:										\
	STORM_COMMON								\
	private:

/**
 * Create classes in storm. Usage: CREATE(Type, engine, param1, param2, ...)
 */
#define CREATE(type, engine, ...)				\
	new (type::stormType(engine)) type(__VA_ARGS__)


#include "Utils/Utils.h"
#include "Utils/Platform.h"
#include "Code/Code.h"

namespace storm {
	using code::Size;
	using code::Offset;

	class Engine;
	class Type;
}

#endif
