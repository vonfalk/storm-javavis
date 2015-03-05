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

	// Compare.
	bool operator <(const Thread &o) const;
};
