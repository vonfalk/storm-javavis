#pragma once

#include "ConUtils.h"
#include <iostream>

#ifdef _DEBUG

// print line to debug output, usage:
// PLN("Hello " << "world!");
// PLN("Hello " << n << " times!");
// PLN_IF("X is larger than 10", X > 10);
#define PLN(str) util::debugStream() << str << std::endl
#define PLN_IF(str, cond) if (cond) PLN(str)
#define PLN_LINES(str) util::printLines(str)
#define PVAR(expr) PLN(#expr << "=" << (expr))

// Good way of making TODO: comments
#define TODO(str) PLN("TODO("__FUNCTION__"):" << str)

// Good way of indicating something possibly interesting during debugging.
#define WARNING(str) PLN("WARNING "__FUNCTION__": " << str);

#include "Timer.h"
#define TIME(str) util::Timer __timer__(str);

#else
#define PLN(str)
#define PLN_IF(str, cond)
#define PVAR(expr)
#define TIME(str)
#define WARNING(str)
#define TODO(str)

#endif

namespace util {
	// set output target (default is automatic depending on configuration)
	enum DebugTarget { debugToAuto, debugToVS, debugToConsole, debugToStdOut };
	void setDebugTarget(DebugTarget target);

	// get the default debug output
	std::wostream &debugStream();

	// print line-numbers on string
	void printLines(const std::string &str);
	void printLines(const String &str);
}