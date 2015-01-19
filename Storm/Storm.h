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
	static Type *type(Object *o);					\
	template <class T>								\
	static inline Type *type(const Auto<T> &o) {	\
		return type(o.borrow());					\
	}												\
	template <class T>								\
	static inline Type *type(const Par<T> &o) {		\
		return type(o.borrow());					\
	}

// Mark a value-type.
#define STORM_VALUE \
	TYPE_CODE		\
private:

// Mark a class(object)-type.
#define STORM_CLASS								\
	TYPE_CODE									\
	static void *cppVTable();					\
private:

// Mark a constructor.
#define STORM_CTOR


namespace storm {
	class Engine;

	using code::Size;
	using code::Offset;
}

