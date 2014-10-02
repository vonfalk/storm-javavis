#pragma once

#include <string>
#include <sstream>
#include <vector>

// String class for use in this project. Adds some neat features to the
// standard std::string.
class String : public std::wstring {
public:
	inline String() {}
	inline String(const std::wstring &str) : std::wstring(str) {}
	inline String(const wchar_t *str) : std::wstring(str) {}
	inline String(nat count, wchar_t ch) : std::wstring(size_t(count), ch) {}

	// Concat ctor
	inline String(const wchar_t *strA, const wchar_t *strB) : std::wstring(strA) {
		*this += strB;
	}

	// Substr ctors
	inline String(const String &original, nat start) : std::wstring(original, start) {}
	inline String(const String &original, nat start, nat length) : std::wstring(original, start, length) {}

	// Conversion to and from ANSI/UTF-8 strings
	explicit String(const char *str);
	std::string toChar() const;

	// Contains anything?
	inline bool any() const { return size() > 0; }

	// Operators on the string.
	inline String operator +(const String &b) const { return String(this->c_str(), b.c_str()); }

	// Compare without case.
	int compareNoCase(const String &str) const;

	// To uppercase
	String toUpper() const;

	// To lowercase
	String toLower() const;

	// Trim leading and/or trailing spaces
	String trim() const;

	// Starts/ends with?
	bool endsWith(const String &ending) const;
	bool startsWith(const String &beginning) const;

	// Starts/ends with, no case
	bool endsWithNC(const String &ending) const;
	bool startsWithNC(const String &beginning) const;

	// Neat functions inspired from the CString in MFC. All are safe in the sense that they
	// will return a result, even if the parameter indicates something strange.
	String left(nat size) const;
	String right(nat size) const;
	inline String mid(nat start) const { return String(*this, start); }
	inline String mid(nat start, nat length) const { return String(*this, start, length); }
	inline String substr(nat start, nat length = npos) const { return String(*this, start, length); }

	// Convert a string to other datatypes.
	double toDouble() const;
	int toInt() const;
	nat toNat() const;
	nat toIntHex() const;

	// Treat the string as a command line.
	String unescape(bool keepUnknown = false) const;
	String firstParam() const;
	String restParams() const;

	// Split string.
	vector<String> split(const String &delim) const;

	// Redefine npos, since sizeof(nat) may not be equal to sizeof(size_t)
	static const nat npos = -1;
};

inline String operator +(const String &a, const wchar_t *b) {
	return String(a.c_str(), b);
}

inline String operator +(const wchar_t *a, const String &b) {
	return String(a, b.c_str());
}

vector<String> split(const String &str, const String &delimiter);
String join(const vector<String> &data, const String &between);

// To string for various datatypes.
String toHex(int i, bool prefix = false);
String toHex(int64 i, bool prefix = false);
String toHex(nat i, bool prefix = false);
String toHex(nat64 i, bool prefix = false);
String toHex(const void *x, bool prefix = true);
String toS(int i);
String toS(nat i);
String toS(int64 i);
String toS(nat64 i);
String toS(double i);

// To string based on the output operator.
template <class T>
String toS(const T &v) {
	std::wostringstream to;
	to << v;
	return to.str();
}

// A join for any type we can do toS on.
template <class T>
String join(const vector<T> &data, const String &between) {
	vector<String> tmp;
	tmp.reserve(data.size());
	for (nat i = 0; i < data.size(); i++) {
		tmp.push_back(toS(data[i]));
	}
	return join(tmp, between);
}

// To string for arrays.
template <class T>
String toS(const vector<T> &data) {
	return join(data, L", ");
}

// Output for arrays.
template <class T>
std::wostream &operator <<(std::wostream &to, const vector<T> &data) {
	return to << toS(data);
}
