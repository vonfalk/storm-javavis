#include "stdafx.h"
#include "Timestamp.h"
#include "Platform.h"
#include <iomanip>

#if defined(WINDOWS)

Timestamp::Timestamp() {
	LARGE_INTEGER li;
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	time = li.QuadPart / 10;
}

Timestamp fromFileTime(FILETIME ft) {
	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	return Timestamp(li.QuadPart / 10);
}

void Timestamp::output(std::wostream &to) const {
	LARGE_INTEGER li;
	li.QuadPart = time * 10LL;
	FILETIME fTime = { li.LowPart, (DWORD)li.HighPart };
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

#elif defined(POSIX)

Timestamp::Timestamp() {
	timespec t;
	clock_gettime(CLOCK_REALTIME, &t);

	time = t.tv_sec * 1000000LL;
	time += t.tv_nsec / 1000;
}

Timestamp fromFileTime(time_t time) {
	return Timestamp(time * 1000000LL);
}

void Timestamp::output(std::wostream &to) const {
	char buf[128];
	time_t time = this->time / 1000000LL;
	tm z;
	localtime_r(&time, &z);
	strftime(buf, 128, "%Y-%m-%d %H:%M:%S", &z);
	to << buf;
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

