#pragma once

#ifdef assert
#undef assert
#endif

#ifdef DEBUG
#define assert(X, ...) if (!(X)) throw AssertionException(WIDEN(__FILE__), __LINE__, WIDEN(#X), __VA_ARGS__)
#define dbg_assert(X, msg) if (!(X)) { PLN((msg)); DebugBreak(); }
#else
#define assert(X, ...)
#define dbg_assert(X, msg)
#endif

#include "Exception.h"

class AssertionException : public Exception {
public:
	AssertionException(const String &file, nat line, const String &expr, const String &msg = L"");
	AssertionException(const String &file, nat line, const String &expr, const char *msg);

	virtual String what() const;

private:
	String file, expr, msg;
	nat line;
};
