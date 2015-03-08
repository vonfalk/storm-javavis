#include "stdafx.h"
#include "Exception.h"

Exception::Exception() : stackTrace(::stackTrace(1)) {
	PLN("Creating " << this);
}

Exception::~Exception() {
	PLN("Deleting " << this);
}

Exception::Exception(const Exception &o) {
	PLN("Copied to " << this);
}

void Exception::output(wostream &to) const {
	to << what();
	to << endl << format(stackTrace);
}
