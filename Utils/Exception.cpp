#include "stdafx.h"
#include "Exception.h"

Exception::Exception() : stackTrace(::stackTrace(1)) {}

Exception::~Exception() {}

void Exception::output(wostream &to) const {
	to << what();
	to << endl << format(stackTrace);
}
