#include "StdAfx.h"
#include "SequenceNumber.h"

namespace util {

	SequenceNumber::SequenceNumber(int startAt) {
		currentNumber = startAt;
		startNumber = startAt;
	}

	SequenceNumber::~SequenceNumber() {}

	SequenceNumber::operator int() {
		return get();
	}

	int SequenceNumber::get() {
		if (currentNumber < startNumber) currentNumber = startNumber;
		return currentNumber++;
	}

	int SequenceNumber::peek() const {
		if (currentNumber < startNumber) return startNumber;
		return currentNumber;
	}

};