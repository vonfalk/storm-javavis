#include "stdafx.h"
#include "CppName.h"

CppName::CppName() {}

CppName::CppName(const String &name) : String(name) {}

CppName CppName::operator +(const String &part) const {
	if (empty())
		return CppName(part);
	else
		return CppName(String::operator +(L"::" + part));
}

static nat lastDots(const String &str) {
	nat z = str.rfind(':');
	if (z == 0 || z == String::npos)
		return String::npos;

	if (str[z-1] == ':')
		return z-1;

	return String::npos;
}

String CppName::last() const {
	nat z = lastDots(*this);
	if (z == String::npos)
		return *this;

	return substr(z + 2);
}

CppName CppName::parent() const {
	nat z = lastDots(*this);
	if (z == String::npos)
		return CppName(L"");
	else
		return CppName(substr(0, z));
}
