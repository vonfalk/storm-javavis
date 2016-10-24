#pragma once
#include "Core/TObject.h"

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
		// TODO
	};

}
