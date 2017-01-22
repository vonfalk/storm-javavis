#include "stdafx.h"
#include "Thread.h"

bool Thread::operator <(const Thread &o) const {
	if (name != o.name)
		return name < o.name;
	return pkg < o.pkg;
}
