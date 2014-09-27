#include "StdAfx.h"
#include "Timestamp.h"

namespace util {

	int64 Timestamp::deltaT = 0;

	Timestamp::FreqMode Timestamp::freqMode = fNotInitialized;
	int64 Timestamp::freqCount;

	Timestamp::Timestamp() {
		//TODO: Use the getTimestamp() static member to get the time in accurate units.

		LARGE_INTEGER li;
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		li.LowPart = ft.dwLowDateTime;
		li.HighPart = ft.dwHighDateTime;

		time = li.QuadPart / 10;
	}

	Timestamp::Timestamp(uint64 t) {
		time = t;
	}

	Timestamp::Timestamp(const void *from) {
		time = *((uint64 *) from) - deltaT;
	}


	bool Timestamp::operator >(const Timestamp &other) const {
		if (time > other.time) {
			return (time - other.time) < wrapLimit;
		} else if (time < other.time) {
			return (other.time - time) > wrapLimit;
		}
		return false;
	}

	bool Timestamp::operator <(const Timestamp &other) const {
		if (time > other.time) {
			return (time - other.time) > wrapLimit;
		} else if (time < other.time) {
			return (other.time - time) < wrapLimit;
		}
		return false;
	}

	bool Timestamp::operator >=(const Timestamp &other) const {
		return ((*this) > other) || ((*this) == other);
	}

	bool Timestamp::operator <=(const Timestamp &other) const {
		return ((*this) > other) || ((*this) == other);
	}


	Timespan Timestamp::operator -(const Timestamp &other) const {
		return Timespan(time - other.time);
	}

	Timestamp Timestamp::operator +(const Timespan &other) const {
		return Timestamp(time + other.time);
	}

	Timestamp Timestamp::operator -(const Timespan &other) const {
		return Timestamp(time - other.time);
	}

	Timestamp &Timestamp::operator +=(const Timespan &other) {
		time += other.time;
		return *this;
	}

	Timestamp &Timestamp::operator -=(const Timespan &other) {
		time -= other.time;
		return *this;
	}

	void Timestamp::output(std::wostream &to) const {
		to << int(time + deltaT);
	}

	void Timestamp::save(void *to) const {
		*((uint64 *)to) = time + deltaT;
	}

	void Timestamp::sync(Timespan deltaT) {
		PLN(L"Synced time as " << deltaT.toS() << L"\n");
		Timestamp::deltaT = deltaT.micros();
	}

	void Timestamp::init() {
		if (freqMode != fNotInitialized) return;

		LARGE_INTEGER li;
		if (QueryPerformanceFrequency(&li) == FALSE) {
			freqMode = fSystemTime;
			freqCount = microsPerSecond * 10L;
		} else {
			freqMode = fPerformanceCounter;
			freqCount = li.QuadPart;
		}
	}

	int64 Timestamp::getTimestamp() {
		init();

		LARGE_INTEGER li;
		switch (freqMode) {
			case fPerformanceCounter:
				QueryPerformanceCounter(&li);
				break;
			case fSystemTime:
				{
					FILETIME ft;
					GetSystemTimeAsFileTime(&ft);
					li.LowPart = ft.dwLowDateTime;
					li.HighPart = ft.dwHighDateTime;
				}
				break;
		}
		return li.QuadPart;
	}

	//int64 Timestamp::toMicros(int64 time) {
	//	init();
	//}

	//int64 Timestamp::fromMicros(int64 time) {
	//	init();
	//}
}
