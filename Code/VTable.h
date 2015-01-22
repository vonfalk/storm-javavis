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

		// Get/set the extra member in the VTable.
		void *extra() const;
		void extra(void *to);

		// Get our size.
		inline nat count() const { return size; }

	private:
		// Size of the vtable.
		nat size;

		// Content of the vtable.
		void **content;

		// Offset of the extra data (system-dependent).
		static int extraOffset;
	};

	// Compute the size of a VTable.
	nat vtableCount(void *vtable);

	// Get the vtable from an object.
	void *vtableOf(void *object);

	// Set the vtable of an object.
	void setVTable(void *object, void *vtable);

}
