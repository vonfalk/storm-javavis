#pragma once

namespace code {

	/**
	 * Handles the platform-specifics of VTable implementations.
	 * The class will read an existing vtable to find out where
	 * function definitions are located, and allows the creation
	 * of a new vtable compatible with the c++ vtable which extends
	 * the base class with custom methods.
	 *
	 * NOTE: This assumes single inheritance in the hierarchy!
	 */
	class VTable : NoCopy {
	public:
		// Create a VTable based on a C++ vtable.
		VTable(void *cppVTable);

		// Dtor.
		~VTable();

		// Set this vtable to an object.
		void setTo(void *object);

		// Set the position indicated with a new function. "fn" is
		// a function pointer to the member to be replaced.
		void set(void *fn, void *newFn);

	private:
		// Size of the vtable.
		nat size;

		// Content of the vtable.
		void **content;

	};

	// Get the vtable from an object.
	void *vtableOf(void *object);

	// Set the vtable of an object.
	void setVtable(void *object, void *vtable);

}
