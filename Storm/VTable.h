#pragma once

namespace code {
	class VTable;
}

namespace storm {

	class Object;

	/**
	 * Implementation of a VTable for storm programs. This implementation
	 * provides either an echo of the compiler generated VTable, or a modified
	 * one that also includes a pointer to the Storm VTable. We can not add
	 * the Storm entries after the regular VTable since we need to relocate
	 * the VTable itself as entries are added or removed, and then we would
	 * need to update the VTable pointer in every live object.
	 *
	 * Therefore, this object has two states. Either it is initialized to echo
	 * a compiler generated VTable, or it has generated its own VTable. In the
	 * latter case, it will also keep track of the Storm VTable. This means that
	 * regular C++ classes can not be extended with Storm members at runtime in
	 * the current implementation since we can not generate and update the VTable
	 * in the C++ constructor.
	 *
	 * Note: Designed as a value since the Type object needs to be able to create
	 * instances of this object at startup.
	 *
	 * TODO: We have trouble when an object switches C++-based base class! In that
	 * case we need to re-allocate the VTable itself... Maybe we can figure out the
	 * maxium size needed and use that?
	 */
	class VTable : NoCopy {
	public:
		// Initialize to empty, use 'create' later.
		VTable(Engine &e);

		// Create from a C++ vtable. This can only be done once.
		void create(void *cppVTable);

		// Create a sub-vtable. This can be done multiple times if the inheritance
		// chain is broken.
		void create(const VTable &parent);

		// Replace the VTable of an object. (silently does nothing if not 'create'd)
		void update(Object *object);

	private:
		// Engine.
		Engine &engine;

		// The original C++ VTable we are currently based off.
		void *cppVTable;

		// The derived VTable (if we have generated one).
		code::VTable *replaced;
	};

}
