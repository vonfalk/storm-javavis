#pragma once
#include "Code/Reference.h"

namespace storm {

	class VTable;
	class Function;

	/**
	 * Updates a single VTable reference. It is owned by the StormVTable (and later whoever
	 * updates C++ vtables). It could be refcounted, but for safety it is not (Type is dangerous).
	 */
	class VTableSlot : public code::Reference {
		friend class StormVTable;
	public:
		VTableSlot(VTable &owner, Function *fn);

		// Called when updated.
		virtual void onAddressChanged(void *addr);

		// Update the vtable now.
		void update();

		// The actual function.
		Function *const fn;

	private:
		// Which VTable to update?
		VTable &owner;

		// The index to update.
		nat id;
	};


}
