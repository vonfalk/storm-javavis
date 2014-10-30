#pragma once

#include "Timestamp.h"

namespace util {

	class Timer : NoCopy {
	public:
		Timer(const char *msg);
		Timer(const wchar_t *msg);
		~Timer();

	private:
		const char *msg;
		const wchar_t *wmsg;
		Timestamp start;
	};

}
