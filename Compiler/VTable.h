#pragma once
#include "Core/TObject.h"
#include "VTableSlot.h"
#include "VTableCpp.h"
#include "VTableStorm.h"
#include "Function.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * The VTable implementation in Storm is split into three files. This one (VTable.h) contains
	 * the logical 'one and only' VTable implementation for a type. However, in Storm a VTable is
	 * split into two parts: the C++ VTable and the Storm VTable.
	 *
	 * The C++ VTable is managed by VTableCpp, and is binary compatible with C++. Storm produces its
	 * own derivatives to be able to extend classes declared in C++, but this has some
	 * limitations. For example, we can not dynamically add slots in the VTable without having to
	 * walk the heap and examine all living objects to see if we need to replace their VTables
	 * whenever the VTable shall be expanded.
	 *
	 * Instead, we store the dynamic part in the Storm VTable, which is managed by VTableStorm. For
	 * types declared in Storm, we store a pointer to the Storm VTable at the unused offset -1 in
	 * the C++ VTable. This allows us to change the Storm VTable for all living objects by simply
	 * altering one pointer. However, this indirection has a small penalty which we might want to
	 * get rid of eventually.
	 */
	class VTable : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create a VTable for a type. Use 'createXxx()' to supply the C++ vtable we shall be
		// derived from.
		STORM_CTOR VTable();

		// Create vtable for a class representing a pure C++ type. In this mode, it is not possible
		// to replace functions in the VTable.
		void createCpp(const void *cppVTable);

		// Create a vtable for a class derived from 'parent'. If done more than once, the current
		// VTable is cleared and then re-initialized.
		void STORM_FN createStorm(VTable *parent);

		// Find the current vtable slot for 'fn' (if any).
		VTableSlot STORM_FN findSlot(Function *fn);

		// Set 'slot' to refer to a specific function.
		void STORM_FN set(VTableSlot slot, Function *fn);

	private:
		// The original C++ VTable we are based off. This can be several levels up the inheritance
		// chain.
		const void *original;

		// The derived C++ VTable (if we created one).
		VTableCpp *cpp;

		// The derived Storm VTable (if we created one).
		VTableStorm *storm;
	};

}
