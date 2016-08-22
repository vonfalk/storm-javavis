#pragma once
#include "Utils/StackTrace.h"
#include "Utils/FnLookup.h"

namespace code {

	class Arena;

	/**
	 * Lookup names, also using an Arena.
	 */
	class ArenaLookup : public CppLookup {
	public:
		// Initialize.
		ArenaLookup(Arena &arena);

		// Format.
		virtual String format(const StackFrame &frame) const;

	private:
		// Arena to use.
		Arena &arena;
	};

}
