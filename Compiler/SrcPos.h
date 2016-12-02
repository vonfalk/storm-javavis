#pragma once
#include "Core/Io/Url.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Source position.
	 */
	class SrcPos {
		STORM_VALUE;
	public:
		// Create unknown position.
		STORM_CTOR SrcPos();

		// Create.
		STORM_CTOR SrcPos(Url *file, Nat pos);

		// File. If unknown, 'file' is null.
		MAYBE(Url *) file;

		// Position inside the file.
		Nat pos;

		// Unknown position?
		Bool STORM_FN unknown() const;

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);
	};

	// Output.
	wostream &operator <<(wostream &to, const SrcPos &p);
	StrBuf &STORM_FN operator <<(StrBuf &to, SrcPos p);
}
