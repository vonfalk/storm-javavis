#pragma once

namespace storm {
	STORM_PKG(core);

	/**
	 * Source position.
	 */
	class SrcPos {
		STORM_VALUE;
	public:
		// TODO: FIXME
		Nat pos;

		void deepCopy(CloneEnv *env);
	};

	// Output.
	wostream &operator <<(wostream &to, const SrcPos &p);
}
