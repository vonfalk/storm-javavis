#include "StdAfx.h"
#include "Timer.h"

namespace util {

	Timer::Timer(const String &msg) : msg(msg) {}

	Timer::~Timer() {
		Timestamp end;

		debugStream() << msg;
		debugStream() << L" in " << end - start << std::endl;
	}

}
