#pragma once
#include "Core/Io/Url.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Source position.
	 *
	 * Describes a range of characters somewhere in the source code.
	 */
	class SrcPos {
		STORM_VALUE;
	public:
		// Create unknown position.
		STORM_CTOR SrcPos();

		// Create.
		STORM_CTOR SrcPos(Url *file, Nat start, Nat end);

		// File. If unknown, 'file' is null.
		MAYBE(Url *) file;

		// Start- and end position in the file.
		Nat start;
		Nat end;

		// Unknown position?
		Bool STORM_FN unknown() const;

		// Any data?
		Bool STORM_FN any() const;

		// Increase the positions.
		SrcPos STORM_FN operator +(Nat v) const;

		// Merge with another range.
		SrcPos STORM_FN extend(SrcPos other) const;

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Compare.
		Bool STORM_FN operator ==(SrcPos o) const;
		Bool STORM_FN operator !=(SrcPos o) const;
	};

	// Output.
	wostream &operator <<(wostream &to, const SrcPos &p);
	StrBuf &STORM_FN operator <<(StrBuf &to, SrcPos p);

}
