#include "StdAfx.h"
#include "Timer.h"

namespace util {

	Timer::Timer(const wchar_t *msg) : msg(null), wmsg(msg) {}

	Timer::Timer(const char *msg) : msg(msg), wmsg(null) {}

	Timer::~Timer() {
		Timestamp end;

		if (msg) debugStream() << msg;
		if (wmsg) debugStream() << wmsg;

		debugStream() << L" in " << end - start << std::endl;
	}

}
