#pragma once
#include "TypeVar.h"

namespace storm {

	/**
	 * Management of the exact layout of member variables within
	 * the type itself.
	 *
	 * Currently, it simply lays out the variables in order of addition,
	 * in the future it is probably desirable to order them depending on
	 * size in order to improve efficiency.
	 */
	class TypeLayout : public Printable {
	public:
		// Add a new variable.
		void add(TypeVar *v);
		void add(NameOverload *o);

		// Get the offset of 'v'.
		Offset offset(Size parentSize, const TypeVar *v) const;

		// Total size of all variables.
		Size size(Size parentSize) const;

		// Equality check. Two layouts are equal if the same variables
		// are laid out in the same places.
		bool operator ==(const TypeLayout &o) const;
		bool operator !=(const TypeLayout &o) const;

		// Output with non-default parameters.
		void format(wostream &to, Size relative = Size()) const;

	protected:
		virtual void output(wostream &to) const;

	private:
		// Map from TypeVar to its offset relative a zero offset.
		typedef hash_map<TypeVar *, Size> OffsetMap;
		OffsetMap offsets;

		// Total size as of now.
		Size total;
	};

}
