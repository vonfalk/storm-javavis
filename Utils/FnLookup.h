#pragma once
#include "StackTrace.h"

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



