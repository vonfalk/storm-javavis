#include "stdafx.h"
#include "Timing.h"
#include "Str.h"
#include "OS/UThread.h"
#include <iomanip>

namespace storm {

#ifdef WINDOWS

	static bool initialized = false;
	static int64 resolution;

	static void initialize() {
		LARGE_INTEGER r;
		QueryPerformanceFrequency(&r);
		resolution = r.QuadPart;
	}

	static int64 now() {
		if (!initialized)
			initialize();

		LARGE_INTEGER value;
		QueryPerformanceCounter(&value);
		int64 v = value.QuadPart;

		// Avoid overflows when 'v' is large. Otherwise we may multiply 'v' with
		// 1000000 and overflow before dividing the number, which effectively reduces
		// the valid range of our return value. This solution reduces that issue.
		int64 seconds = v / resolution;
		int64 remaining = v % resolution;
		const int64 s = 1000 * 1000;

		return seconds * s + (remaining * s) / resolution;
	}

#else
#error "Please implement some time-keeping for your platform."
#endif

	Moment::Moment() : v(now()) {}

	wostream &operator <<(wostream &to, Moment m) {
		return to << L"@" << m.v << L" us";
	}

	wostream &operator <<(wostream &to, Duration d) {
		int64 t = abs(d.v);

		struct Unit {
			const wchar *name;
			int64 factor;
		};

		static Unit units[] = {
			{ L"us", 1 },
			{ L"ms", 1000 },
			{ L"s", 1000 },
			{ L"min", 60 },
			{ L"h", 60 },
			{ L"d", 24 },
		};

		int64 div = units[0].factor;
		const wchar *unit = units[0].name;

		for (nat i = 1; i < ARRAY_COUNT(units); i++) {
			if ((t / div) < units[i].factor)
				break;

			div *= units[i].factor;
			unit = units[i].name;
		}

		return to << std::fixed << std::setprecision(2) << (d.v / double(div)) << L" " << unit;
	}

	Str *toS(EnginePtr e, Moment m) {
		return CREATE(Str, e.v, ::toS(m));
	}

	Str *toS(EnginePtr e, Duration d) {
		return CREATE(Str, e.v, ::toS(d));
	}

	Duration h(Long v) {
		return Duration(int64(v) * int64(60) * int64(60) * int64(1000) * int64(1000));
	}

	Duration min(Long v) {
		return Duration(int64(v) * int64(60) * int64(1000) * int64(1000));
	}

	Duration s(Long v) {
		return Duration(int64(v) * int64(1000) * int64(1000));
	}

	Duration ms(Long v) {
		return Duration(int64(v) * 1000);
	}

	Duration us(Long v) {
		return Duration(v);
	}

	void sleep(Duration d) {
		os::UThread::sleep(nat(d.inMs()));
	}

}
