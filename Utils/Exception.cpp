#include "stdafx.h"
#include "Exception.h"

void Exception::output(wostream &to) const {
	to << what();
}
