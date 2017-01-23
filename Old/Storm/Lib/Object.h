#pragma once
// Include the shared implementation.
#include "Shared/Object.h"
#include "Shared/DllInterface.h"
#include "Code/FnParams.h"
#include "Code/Size.h"

namespace storm {

	/**
	 * Create an object without a type marker. This is only for use in early
	 * startup of the compiler, and some objects will crash when using this
	 * kind of initialization. The actual type should be provided later
	 * by using SET_TYPE_LATE.
	 * Example:
	 * Auto<X> foo = CREATE_NOTYPE(X, e, ...);
	 * ...
	 * SET_TYPE_LATE(foo, <type>);
	 *
	 * Note to self: CREATE_NOTYPE is not entirely exception safe. That is OK, since
	 * it is only used once per engine creation.
	 */
#define CREATE_NOTYPE(tName, engine, ...)						\
	new (noTypeAlloc(engine, sizeof(tName))) tName(__VA_ARGS__)

#define SET_TYPE_LATE(object, type)				\
	setTypeLate(object, type)

	// Set the type late. See SET_TYPE_LATE.
	void setTypeLate(Par<Object> o, Par<Type> t);

	// Alloc without type.
	Object::unsafe noTypeAlloc(Engine &e, size_t size);

	// Create an object using the supplied constructor.
	Object *createObj(Type *type, const void *ctor, os::FnParams params);
	Object *createObj(Function *ctor, os::FnParams params);

	template <class T>
	inline T *create(Par<Function> ctor, const os::FnParams &params) {
		return (T *)createObj(ctor.borrow(), params);
	}

	// Create a shallow copy of an object by calling the provided copy ctor.
	Object *createCopy(const void *copyCtor, Object *original);

	void *CODECALL stormMalloc(Type *type);
	void CODECALL stormFree(void *mem);
	void setVTable(Object *o);

	// Size of Object.
	Size objectBaseSize();

	// Size of TObject.
	Size tObjectBaseSize();

	// Offset of TObject::thread.
	Offset tObjectThreadOffset();


	// Get a DllInterface for us! Note that 'builtIn' nor 'data' are set from here!
	DllInterface dllInterface();

}
