#pragma once
#include "Size.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * A variable inside a listing.
	 */
	class Variable {
		STORM_VALUE;
	public:
		inline Bool STORM_FN operator ==(Variable o) const { return id == o.id; }
		inline Bool STORM_FN operator !=(Variable o) const { return id != o.id; }

		// Get the size of our variable.
		inline Size STORM_FN size() const { return sz; }

	private:
		friend class Listing;
		friend wostream &operator <<(wostream &to, Variable v);
		friend StrBuf &operator <<(StrBuf &to, Variable l);

		Variable(Nat id, Size size);

		Nat id;
		Size sz;
	};

	wostream &operator <<(wostream &to, Variable v);
	StrBuf &STORM_FN operator <<(StrBuf &to, Variable l);
}
