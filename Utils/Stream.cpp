#include "stdafx.h"
#include "Stream.h"

Stream::Stream() : hasError(false) {}

Stream::~Stream() {}

nat Stream::read(nat size, void *to) {
	assert(false);
	return 0;
}

void Stream::write(nat size, const void *from) {
	assert(false);
}

bool Stream::more() const {
	if (!valid()) return false;
	return pos() < length();
}


bool Stream::error() const {
	return hasError;
}

void Stream::clearError() {
	hasError = false;
}
