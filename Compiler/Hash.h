#pragma once

namespace storm {

	/**
	 * Hash functions for the built-in types.
	 */
	Nat CODECALL byteHash(Byte v);
	Nat CODECALL intHash(Int v);
	Nat CODECALL natHash(Nat v);
	Nat CODECALL longHash(Long v);
	Nat CODECALL wordHash(Word v);

}
