#include "stdafx.h"
#include "Exception.h"

Exception::Exception() : stackTrace(code::stackTrace(1)) {}

void Exception::output(wostream &to) const {
	to << what();
	to << endl << code::format(stackTrace);
}
