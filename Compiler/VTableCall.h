#pragma once
#include "VTableSlot.h"
#include "Value.h"
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
		code::RefSource *get(VTableSlot slot, Value result);

	private:
		// VTable calls for C++.
		Array<code::RefSource *> *cpp;

		// VTable calls for Storm.
		Array<code::RefSource *> *storm;

		// Number of variants.
		Nat variants;

		// Get a C++ offset.
		code::RefSource *getCpp(Nat offset, Nat id);

		// Get a Storm offset.
		code::RefSource *getStorm(Nat offset, Nat id);

		// Find an entry in the array, expanding the array if necessary.
		code::RefSource *&find(Array<code::RefSource *> *in, Nat offset, Nat id);
	};


	/**
	 * Custom reference sources for the vtable thunks.
	 */
	class VTableSource : public code::RefSource {
		STORM_CLASS;
	public:
		STORM_CTOR VTableSource(VTableSlot slot, Nat id, code::Content *c);

		virtual Str *STORM_FN title() const;

	private:
		VTableSlot slot;
		Nat id;
	};

}
