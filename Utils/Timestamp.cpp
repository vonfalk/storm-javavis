#include "StdAfx.h"
#include "Timestamp.h"
#include "Platform.h"
#include <iomanip>

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
	return ((*this) < other) || ((*this) == other);
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
	LARGE_INTEGER li;
	li.QuadPart = time * 10LL;
	FILETIME fTime = { li.LowPart, li.HighPart };
	SYSTEMTIME sTime;
	FileTimeToSystemTime(&fTime, &sTime);

	using std::setw;

	wchar_t f = to.fill('0');
	to << setw(4) << sTime.wYear << L"-"
	   << setw(2) << sTime.wMonth << L"-"
	   << setw(2) << sTime.wDay << L" "
	   << setw(2) << sTime.wHour << L":"
	   << setw(2) << sTime.wMinute << L":"
	   << setw(2) << sTime.wSecond << L","
	   << setw(4) << sTime.wMilliseconds;
	to.fill(f);
}

