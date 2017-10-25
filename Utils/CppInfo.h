#pragma once
#include "StackInfo.h"

/**
 * Basic lookup, using debug information if available.
 */
class CppInfo : public StackInfo {
public:
	// Format a function call.
	virtual bool format(std::wostream &to, const StackFrame &frame) const;
};
