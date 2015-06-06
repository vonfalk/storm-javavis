#pragma once

namespace code {

	/**
	 * Handles the platform-specifics of VTable implementations.
	 * The class will read an existing vtable to find out where
	 * function definitions are located, and allows the creation
	 * of a new vtable compatible with the c++ vtable which extends
	 * the base class with custom methods.
	 *
	 * The VTable class also provides an additional field in the VTable
	 * that can be used for arbitary storage. This is accessed with the
	 * 'extra' member functions. This extra data will always be at the
	 * same offset for all VTables in the system, regardless of their content.
	 *
	 * NOTE: This assumes single inheritance in the hierarchy!
	 */
	class VTable : NoCopy {
		friend void *vtableExtra(void *);
		friend void *vtableDtor(void *);
	public:
		// Create a VTable based on a C++ vtable. Optionally allocate extra data to be able to
		// replace the VTable later.
		VTable(void *cppVTable, nat count = 0);

		// Create a copy of another VTable.
		explicit VTable(const VTable &o);

		// Update with new data (without replacing the underlying array if possible).
		void replace(void *cppVTable);
		void replace(const VTable &o);

		// Dtor.
		~VTable();

		// Set this vtable to an object.
		void setTo(void *object);

		// Set the position indicated with a new function. "fn" is
		// a function pointer to the member to be replaced.
		void set(void *fn, void *newFn);
		void set(nat slot, void *fn);

		// Get the pointer at a specific location.
		void *get(nat slot);

		// Set the destructor to a new function (dtors are tricky, you can not take the address of them!)
		void setDtor(void *fn);

		// Find the slot of the pointer. The pointer may or may
		// not be previously de-virtualized.
		nat find(void *fn);

		// Get/set the extra member in the VTable.
		void *extra() const;
		void extra(void *to);

		// Get our size.
		inline nat count() const { return size; }

		// Get the pointer.
		inline void *ptr() const { return content; }

		// Invalid slot.
		static const nat invalid = -1;

	private:
		// Size of the vtable.
		nat size;

		// Content of the vtable.
		void **content;

		// Offset of the extra data (system-dependent).
		static int extraOffset;

		// Offset of the dtor (system-dependent).
		static int dtorOffset;
	};

	// De-virtualize a function call. The standard way of calling virtual
	// functions by pointer is to call by vtable. There is no easy way to
	// get the address to the specific function.
	// If 'fnPtr' is not a virtual lookup stub, null is returned.
	void *deVirtualize(void *fnPtr, void *vtable);

	// Find an entry in a vtable. 'fn' may or may not be de-virtualized.
	nat findSlot(void *fn, void *vtable, nat size = 0);

	// Get a slot in a vtable.
	void *getSlot(void *vtable, nat slot);

	// Compute the size of a VTable.
	nat vtableCount(void *vtable);

	// Get the vtable from an object.
	void *vtableOf(const void *object);

	// Get the 'extra' field of the vtable.
	void *vtableExtra(void *vtable);

	// Get the 'dtor' field of the vtable.
	void *vtableDtor(void *vtable);

	// Set the vtable of an object.
	void setVTable(void *object, void *vtable);

}
