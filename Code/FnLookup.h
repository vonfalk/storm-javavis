#pragma once
#include "StackTrace.h"

namespace code {

	class Arena;

	/**
	 * Lookup function names from their addresses.
	 *
	 * Default implementation: only knows about C++ generated functions if
	 * there is debug information present.
	 */
	class FnLookup : NoCopy {
	public:
		// Format a function call.
		virtual String format(const StackFrame &frame) const = 0;
	};

	/**
	 * Basic lookup, using debug information if available.
	 */
	class CppLookup : public FnLookup {
	public:
		// Format a function call.
		virtual String format(const StackFrame &frame) const;
	};


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
