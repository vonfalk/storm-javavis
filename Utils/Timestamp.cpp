#include "StdAfx.h"
#include "Timestamp.h"
#include "Platform.h"

Timestamp::Timestamp() {
	LARGE_INTEGER li;
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	time = li.QuadPart / 10;
}

#ifdef WINDOWS
Timestamp fromFileTime(FILETIME ft) {
	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	return Timestamp(li.QuadPart / 10);
}
#endif

Timestamp::Timestamp(uint64 t) {
	time = t;
}

bool Timestamp::operator >(const Timestamp &other) const {
	return time > other.time;
}

bool Timestamp::operator <(const Timestamp &other) const {
	return time < other.time;
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
	to << int(time);
}

