#include "stdafx.h"
#include "Assert.h"

AssertionException::AssertionException(const String &file, nat line, const String &expr, const String &msg)
	: file(file), line(line), expr(expr), msg(msg) {
	// If no one catches it, print it now as well.
	PLN(what());
	PLN(format(stackTrace));
}

AssertionException::AssertionException(const String &file, nat line, const String &expr, const char *msg)
	: file(file), line(line), expr(expr), msg(String(msg)) {
	// If no one catches it, print it now as well.
	PLN(what());
	PLN(format(stackTrace));
}

String AssertionException::what() const {
	std::wostringstream s;
	s << L"Assertion failed: ";
	if (!msg.empty())
		s << msg << L" (" << expr << L")";
	else
		s << expr;
	s << endl;

	s << file << L"(L" << line << L")";
	return s.str();
}


void debugAssertFailed(const wchar_t *msg) {
	PLN(msg);
	debugAssertFailed();
}

void debugAssertFailed(const String &msg) {
	PLN(msg);
	debugAssertFailed();
}

#ifdef VISUAL_STUDIO

void debugAssertFailed() {
	DebugBreak();
	std::terminate();
}

#else

void debugAssertFailed() {
	std::terminate();
}

#endif
