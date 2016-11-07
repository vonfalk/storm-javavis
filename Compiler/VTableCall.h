#pragma once
#include "VTableSlot.h"
#include "Core/Array.h"
#include "Code/RefSource.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Class which keeps track of all vtable call stubs so that we do not have to duplicate them.
	 */
	class VTableCalls : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		STORM_CTOR VTableCalls();

		// Get a vtable call stub for the desired slot.
		code::RefSource *get(VTableSlot slot);

	private:
		// VTable calls for C++.
		Array<code::RefSource *> *cpp;

		// VTable calls for Storm.
		Array<code::RefSource *> *storm;

		// Get a C++ offset.
		code::RefSource *getCpp(Nat offset);

		// Get a Storm offset.
		code::RefSource *getStorm(Nat offset);
	};

}
