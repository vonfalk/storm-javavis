#pragma once

namespace storm {

	/**
	 * ADT describing a position in a VTable.
	 */
	class VTablePos {
		friend wostream &operator <<(wostream &to, const VTablePos &pos);
	public:

		// Offset?
		nat offset;

		// Type?
		enum Type {
			tNone, tStorm, tCpp
		};
		Type type;

		// Ctor.
		inline VTablePos(nat offset, Type t) : offset(offset), type(t) {}

		// Invalid value.
		inline VTablePos() : offset(0), type(tNone) {}

		// Compare in if()
		inline operator bool() {
			return type != tNone;
		}

		static inline VTablePos storm(nat id) { return VTablePos(id, tStorm); }
		static inline VTablePos cpp(nat id) { return VTablePos(id, tCpp); }
	};

	wostream &operator <<(wostream &to, const VTablePos &pos);




}
