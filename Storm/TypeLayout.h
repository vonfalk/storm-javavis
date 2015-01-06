#pragma once
#include "TypeVar.h"

namespace storm {

	/**
	 * Management of the exact layout of member variables within
	 * the type itself.
	 */
	class TypeLayout {
	public:
		// Add a new variable.
		void add(TypeVar *v);
		void add(NameOverload *o);

		// Get the offset of 'v'.
		nat offset(TypeVar *v) const;

		// Total size of all variables.
		nat size() const;

	private:
		// Offset we're allowed to start from.
		nat baseOffset;
	};

}
