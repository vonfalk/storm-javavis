#pragma once

namespace storm {

	/**
	 * Hash functions for the built-in types.
	 */

	Nat byteHash(Byte v);
	Nat intHash(Int v);
	Nat natHash(Nat v);
	Nat longHash(Long v);
	Nat wordHash(Word v);

}
