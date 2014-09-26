#pragma once

#include "Timespan.h"
#include "Printable.h"

//TODO: ersätt initTickCount, getTickCount samt getNanoTime (i stdafx.cpp) med medlemmar i denna klass!

namespace util {
	class Timespan;

	class Timestamp : public Printable {
	public:
		Timestamp(); //Skapar en ny timestamp av "nu".
		Timestamp(const void *from);

		inline bool operator ==(const Timestamp &other) const { return time == other.time; }
		inline bool operator !=(const Timestamp &other) const { return time != other.time; }
		bool operator >(const Timestamp &other) const;
		bool operator <(const Timestamp &other) const;
		bool operator >=(const Timestamp &other) const;
		bool operator <=(const Timestamp &other) const;

		Timespan operator -(const Timestamp &other) const;
		Timestamp operator +(const Timespan &other) const;
		Timestamp operator -(const Timespan &other) const;
		Timestamp &operator +=(const Timespan &other);
		Timestamp &operator -=(const Timespan &other);

		void save(void *to) const;

		static void sync(Timespan deltaT);

		static const int size = sizeof(int64);

		friend class Timespan;
	protected:
		virtual void output(std::wostream &to) const;
	private:
		Timestamp(uint64 t);
		static const int64 maxTime = 0xFFFFFFFFFFFFFFFF;
		static const int64 wrapLimit = 0x7FFFFFFFFFFFFFFF;
		int64 time; //I millisekunder

		static int64 deltaT;

		enum FreqMode { fNotInitialized, fPerformanceCounter, fSystemTime };

		static const int64 microsPerSecond = 1000000L;
		static FreqMode freqMode;
		static int64 freqCount;

		static void init();
		static int64 getTimestamp();

		//static int64 toMicros(int64 time);
		//static int64 fromMicros(int64 time);
	};
}