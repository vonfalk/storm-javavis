#pragma once
#include "Object.h"
#include "Int.h"

namespace storm {

	STORM_PKG(core);

	/**
	 * The string type used by the generated code.
	 */
	class Str : public Object {
		STORM_CLASS;
	public:
		// The value of this 'str' object.
		String v;

		// Empty ctor
		STORM_CTOR Str();

		// Copy ctor
		STORM_CTOR Str(Par<Str> copy);

		// Create from regular string.
		Str(const String &s);

		// String length.
		Nat STORM_FN count() const;

		// Equals.
		virtual Bool STORM_FN equals(Par<Object> o);

		// ToS
		virtual Str *STORM_FN toS();

	protected:
		virtual void output(wostream &to) const;
	};

}
