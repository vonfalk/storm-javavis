#pragma once

namespace storm {

	/**
	 * ADT describing a position in a VTable.
	 */
	class VTablePos {
		friend class VTableCalls;
		friend class VTable;
		friend wostream &operator <<(wostream &to, const VTablePos &pos);

		// Offset?
		nat offset;

		// Entry in Storm?
		bool stormEntry;

		// Ctor.
		inline VTablePos(nat offset, bool storm) : offset(offset), stormEntry(storm) {}
	};

	wostream &operator <<(wostream &to, const VTablePos &pos);




}
