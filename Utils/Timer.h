#pragma once

#include "Timestamp.h"

namespace util {

	class Timer : NoCopy {
	public:
		Timer(const String &msg);
		~Timer();

	private:
		String msg;
		Timestamp start;
	};

}
