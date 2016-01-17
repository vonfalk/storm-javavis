#include "stdafx.h"
#include "Printable.h"

#include <sstream>

Printable::~Printable() {}

String toS(const Printable &from) {
	std::wostringstream ss;
	ss << from;
	return String(ss.str());
}

std::wostream &operator <<(std::wostream &to, const Printable &from) {
	from.output(to);
	return to;
}


