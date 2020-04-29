#pragma once
#include "Core/Io/Protocol.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Protocol for files inside the compiler.
	 */
	class CompilerProtocol : public Protocol {
		STORM_CLASS;
	public:
		// Create. Give the name of the library, if any.
		STORM_CTOR CompilerProtocol();
		STORM_CTOR CompilerProtocol(Str *libName);

		// Compare two parts of a filename for equality.
		// Implemented here as a simple bitwise comparision.
		virtual Bool STORM_FN partEq(Str *a, Str *b);

		// Hash a part of a filename.
		virtual Nat STORM_FN partHash(Str *a);

		// Convert to a string suitable for other C-api:s
		virtual Str *STORM_FN format(Url *url);

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// The same protocol as some other?
		virtual Bool STORM_FN operator ==(const Protocol &o) const;

	private:
		// Library name (if any).
		MAYBE(Str *) libName;
	};

}
