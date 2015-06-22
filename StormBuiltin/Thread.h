#pragma once
#include "CppName.h"

class Thread {
public:

	// Name in C++.
	CppName cppName;

	// Name in storm.
	String name;

	// Package in storm.
	String pkg;

	// External?
	bool external;

	// Compare.
	bool operator <(const Thread &o) const;
};
