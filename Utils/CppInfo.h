#pragma once
#include "StackInfo.h"

/**
 * Basic lookup, using debug information if available.
 */
class CppInfo : public StackInfo {
public:
	// Collect data.
	virtual bool translate(void *ip, void *&fnBase, int &offset) const;

	// Format a function call.
	virtual void format(GenericOutput &to, void *fnBase, int offset) const;
};
