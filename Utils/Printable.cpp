#include "StdAfx.h"
#include "Printable.h"

#include <sstream>

namespace util {

	Printable::~Printable() {}

	String Printable::toString() const {
		std::wostringstream ss;
		ss << *this;
		return String(ss.str());
	}

	std::wostream &operator <<(std::wostream &to, const Printable &from) {
		from.output(to);
		return to;
	}

}