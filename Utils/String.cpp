#include "stdafx.h"
#include "String.h"

#include <sstream>
#include <iomanip>

String::String(const char *chars) {
	nat size = strlen(chars);
	// Allocate memory for the worst case.
	nat bufferSize = size * 2 + 1;
	wchar_t *outBuffer = new wchar_t[bufferSize];
	outBuffer[0] = 0;

	size_t outSize = 0;
	errno_t error = mbstowcs_s(&outSize, outBuffer, bufferSize, chars, bufferSize - 1);
	assert(error == 0);

	*this = outBuffer;

	delete []outBuffer;
}

std::string String::toChar() const {
	// Worst case is 4 chars per wchar_t in UTF-8
	nat bufferSize = size() * 4 + 1;
	char *outBuffer = new char[bufferSize];
	outBuffer[0] = 0;

	size_t outSize = 0;
	errno_t error = wcstombs_s(&outSize, outBuffer, bufferSize, c_str(), bufferSize - 1);
	assert(error == 0);

	std::string result(outBuffer);
	delete []outBuffer;

	return result;
}

String String::left(nat size) const {
	return String(*this, 0, size);
}

String String::right(nat size) const {
	nat start = 0;
	if (size < this->size()) start = this->size() - size;
	return String(*this, start);
}

int String::compareNoCase(const String &str) const {
	return _wcsicmp(c_str(), str.c_str());
}

bool String::startsWith(const String &str) const {
	return wcsncmp(c_str(), str.c_str(), str.size()) == 0;
}

bool String::endsWith(const String &str) const {
	if (size() < str.size()) return false;
	nat delta = size() - str.size();
	const wchar_t *ourStart = c_str() + delta;
	return wcscmp(ourStart, str.c_str()) == 0;
}

bool String::startsWithNC(const String &str) const {
	return _wcsnicmp(c_str(), str.c_str(), str.size()) == 0;
}

bool String::endsWithNC(const String &str) const {
	if (size() < str.size()) return false;
	nat delta = size() - str.size();
	const wchar_t *ourStart = c_str() + delta;
	return _wcsicmp(ourStart, str.c_str()) == 0;
}

String String::toUpper() const {
	wchar_t *buffer = new wchar_t[size() + 1];
	wcscpy_s(buffer, size() + 1, c_str());
	_wcsupr_s(buffer, size() + 1);
	String result(buffer);
	delete []buffer;
	return result;
}

String String::toLower() const {
	wchar_t *buffer = new wchar_t[size() + 1];
	wcscpy_s(buffer, size() + 1, c_str());
	_wcslwr_s(buffer, size() + 1);
	String result(buffer);
	delete []buffer;
	return result;
}

String String::trim() const {
	if (size() == 0) return L"";

	nat start = 0, end = size() - 1;
	for (start = 0; start < size(); start++) {
		if (!isspace((*this)[start])) break;
	}

	while (end > start) {
		if (!isspace((*this)[end])) break;
		if (end > 1) --end;
	}

	if (start > end) return L"";
	return substr(start, end - start + 1);
}

vector<String> String::split(const String &delim) const {
	return ::split(*this, delim);
}

vector<String> split(const String &str, const String &delimiter) {
	vector<String> r;

	nat start = 0;
	nat end = str.find(delimiter);
	while (end != String::npos) {
		r.push_back(str.substr(start, end - start));

		start = end + delimiter.size();
		end = str.find(delimiter, start);
	}

	if (start < str.size())
		r.push_back(str.substr(start));

	return r;
}

String toHex(const void *x, bool prefix) {
	if (sizeof(void *) == sizeof(nat)) {
		return toHex(nat(x), prefix);
	} else if (sizeof(void *) == sizeof(nat64)) {
		return toHex(nat64(x), prefix);
	}
}

String toHex(int x, bool prefix) {
	std::wostringstream oss;

	if (prefix) oss << L"0x";
	oss << std::hex << std::setw(CHAR_BIT / 4) << std::setfill(L'0') << x;

	return oss.str();
}

String toHex(int64 x, bool prefix) {
	std::wostringstream oss;

	if (prefix) oss << L"0x";
	oss << std::hex << std::setw(CHAR_BIT / 4) << std::setfill(L'0') << x;

	return oss.str();
}

String toHex(nat x, bool prefix) {
	std::wostringstream oss;

	if (prefix) oss << L"0x";
	oss << std::hex << std::setw(CHAR_BIT / 4) << std::setfill(L'0') << x;

	return oss.str();
}

String toHex(nat64 x, bool prefix) {
	std::wostringstream oss;

	if (prefix) oss << L"0x";
	oss << std::hex << std::setw(CHAR_BIT / 4) << std::setfill(L'0') << x;

	return oss.str();
}


template <class T>
inline String genericToS(T t) {
	std::wostringstream oss;
	oss << t;
	return oss.str();
}

String toS(int i) { return genericToS(i); }
String toS(nat i) { return genericToS(i); }
String toS(int64 i) { return genericToS(i); }
String toS(nat64 i) { return genericToS(i); }
String toS(double i) { return genericToS(i); }
const String &toS(const String &s) { return s; }

double String::toDouble() const {
	wchar_t *end;
	return wcstod(c_str(), &end);
}

int String::toInt() const {
	wchar_t *end;
	return wcstol(c_str(), &end, 10);
}

nat String::toNat() const {
	wchar_t *end;
	return wcstoul(c_str(), &end, 10);
}

int64 String::toInt64() const {
	wchar_t *end;
	return _wcstoi64(c_str(), &end, 10);
}

nat64 String::toNat64() const {
	wchar_t *end;
	return _wcstoui64(c_str(), &end, 10);
}

nat String::toIntHex() const {
	wchar_t *end;
	if (left(2) == L"0x") {
		return wcstol(mid(2).c_str(), &end, 16);
	} else {
		return wcstol(c_str(), &end, 16);
	}
}

bool String::isInt() const {
	if (size() == 0)
		return false;

	nat i = 0;
	if ((*this)[i] == '-')
		i = 1;

	for (; i < size(); i++) {
		wchar c = (*this)[i];
		if (c < '0' || c > '9')
			return false;
	}
	return true;
}

bool String::isNat() const {
	for (nat i = 0; i < size(); i++) {
		wchar c = (*this)[i];
		if (c < '0' || c > '9')
			return false;
	}
	return true;
}

String String::unescape(bool keepUnknown) const {
	std::wostringstream ret;

	const String &str = *this;

	for (nat i = 0; i < str.size(); i++) {
		wchar_t ch = str[i];
		if (str[i] == '\\') {
			switch (str[++i]) {
				case 'n':
					ret << L'\n';
					break;
				case 'r':
					ret << '\r';
					break;
				case 't':
					ret << '\t';
					break;
				case 'v':
					ret << '\v';
					break;
				case 'x':
					//till hexadecimal...
					ret << wchar_t(str.mid(i + 1, 4).toIntHex());
					i += 4;
					break;
				default:
					if (keepUnknown)
						ret << '\\';
					ret << str[i];
					break;
			}
		} else {
			ret << str[i];
		}
	}

	return ret.str().c_str();
}

String String::escape() const {
	std::wostringstream to;
	for (nat i = 0; i < size(); i++)
		to << escape((*this)[i]);
	return to.str();
}

String String::escape(wchar c) {
	switch (c) {
	case '\n':
		return L"\\n";
	case '\r':
		return L"\\r";
	case '\t':
		return L"\\t";
	default:
		return String(1, c);
	}
}

static nat firstParameterEnd(const String &str) {
	bool content = false;
	bool inString = false;
	bool lastQuoted = false;
	for (nat i = 0; i < str.size(); i++) {
		bool quote = false;
		switch (str[i]) {
			case '"':
				if (content) {
					if (!lastQuoted) return i + (inString ? 1 : 0);
				} else {
					if (!lastQuoted) inString = !inString;
				}
				break;
			case ' ':
				if (!inString) if (content) return i;
				break;
			case '\\':
				content = true;
				quote = true;
				break;
			default:
				content = true;
				break;
		}
		lastQuoted = quote;
	}

	return str.size();
}

String String::firstParam() const {
	int end = firstParameterEnd(*this);

	String toReturn = left(end).trim();

	if ((toReturn[0] == '"') && (toReturn[toReturn.size() - 1] == '"')) {
		return toReturn.mid(1, toReturn.size() - 2);
	}
	return toReturn;
}

String String::restParams() const {
	int end = firstParameterEnd(*this);

	if (end == size()) return L"";

	String toReturn = mid(end).trim();
	return toReturn;
}
