#pragma once

#ifdef assert
#undef assert
#endif

#ifdef DEBUG
#define assert(X, ...) if (!(X)) throw AssertionException(WIDEN(__FILE__), __LINE__, WIDEN(#X), __VA_ARGS__)
#else
#define assert(X, ...) do {} while (false)
#endif

#include "Exception.h"

class AssertionException : public Exception {
public:
	AssertionException(const String &file, nat line, const String &expr, const String &msg = L"");

	virtual String what() const;

private:
	String file, expr, msg;
	nat line;
};
