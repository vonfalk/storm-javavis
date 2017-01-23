#pragma once
#include "Code/Value.h"
#include "StormVTable.h"
#include "VTablePos.h"

namespace code {
	class VTable;
	class Binary;
}

namespace storm {

	class Object;
	class Function;

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
	 * TODO: This also contains the same information as the chain class, remove duplication?
	 */
	class VTable : NoCopy {
		// Needed to update actual data.
		friend class VTableSlot;
	public:
		// Initialize to empty, use 'create' later.
		VTable(Engine &e);

		// Dtor.
		~VTable();

		// Clear any references we have.
		void clearRefs();

		// Create from a C++ vtable. This can only be done once. If you want a Storm VTable, use parent() only.
		void createCpp(void *cppVTable);

		// Set the parent of this VTable. If this is not initialized using 'create', a sub-vtable is
		// created to mirror the parent vtable.
		void setParent(VTable &parent);

		// Get the C++ vtable we're based off.
		inline void *baseVTable() const { return cppVTable; }

		// Create a new root-vtable for an object not inherited from anything.
		void create();

		// Replace the VTable of an object. (silently does nothing if not 'create'd)
		void update(Object *object);

		// The reference to the vtable itself.
		code::RefSource ref;

		// Built in vtable?
		bool builtIn() const;

		// Insert a function into the VTable.
		VTablePos insert(Function *fn);

		// Debug output of this VTable and any children.
		void dbg_dump();

	private:
		// Engine.
		Engine &engine;

		// The original C++ VTable we are currently based off.
		void *cppVTable;

		// The derived VTable (if we have generated one).
		code::VTable *replaced;

		// Functions we have replaced in 'replaced', or 'cppVTable'.
		vector<VTableSlot *> cppSlots;

		// Storm VTable.
		StormVTable storm;

		// Known children.
		typedef set<VTable *> ChildSet;
		ChildSet children;

		// Our parent.
		VTable *parent;

		// Update the address for entry 'i'.
		void updateAddr(VTablePos i, void *to, VTableSlot *src);

		// Find a base-variation of the function 'fn', return its index.
		VTablePos findBase(Function *fn);
		VTablePos findCppBase(Function *fn);
		VTablePos findStormBase(Function *fn);

		// Find a suitable slot for 'fn'.
		VTablePos findSlot(Function *fn);

		// Find a suitable slot for 'dtor'.
		VTablePos findDtorSlot(Function *fn);

		// Check if 'fn' was inserted here earlier.
		bool inserted(Function *fn);

		// Resize requested from the parent.
		void expand(nat into, nat count);
		void contract(nat from, nat to);

		// Slot get/set.
		VTableSlot *slot(VTablePos pos);
		void slot(VTablePos pos, VTableSlot *to);

		// Address set.
		void addr(VTablePos pos, void *to);

		// Update the Storm vtable.
		void updateStorm(VTable &parent);
	};


	/**
	 * Keep track of and share created vtable call stubs.
	 */
	class VTableCalls : NoCopy {
	public:
		// Create
		VTableCalls(Engine &e);

		// Dtor.
		~VTableCalls();

		// Get the vtable call for entry pos.
		code::Ref call(VTablePos pos);

	private:
		// Engine.
		Engine &engine;

		// Created code stubs.
		vector<code::RefSource *> stormCreated;
		vector<code::RefSource *> cppCreated;

		// Get the call.
		code::Ref call(nat i, vector<code::RefSource *> &src, code::RefSource *(VTableCalls::*create)(nat));

		// Create a stub.
		code::RefSource *createStorm(nat i);
		code::RefSource *createCpp(nat i);
	};

}