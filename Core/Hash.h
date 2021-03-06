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
	Nat CODECALL ptrHash(const void *v);


	// Check so a handle can be used for a hash map.
	class Handle;
	void checkHashHandle(const Handle &h);
}
