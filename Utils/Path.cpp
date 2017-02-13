#include "stdafx.h"
#include "Path.h"
#include "Exception.h"

#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

//////////////////////////////////////////////////////////////////////////
// The Path class
//////////////////////////////////////////////////////////////////////////

Path Path::executableFile() {
	static Path e;
	if (e.parts.size() == 0) {
		wchar_t tmp[MAX_PATH + 1];
		GetModuleFileName(NULL, tmp, MAX_PATH + 1);
		e = Path(tmp);
	}
	return e;
}

Path Path::executable() {
	static Path e;
	if (e.parts.size() == 0)
		e = executableFile().parent();
	return e;
}

Path Path::executable(const Path &path) {
	return executable() + path;
}

Path Path::dbgRoot() {
#ifndef DEBUG
	WARNING(L"Using dbgRoot in release!");
#endif
	return executable().parent();
}

Path Path::cwd() {
	wchar_t tmp[MAX_PATH + 1];
	_wgetcwd(tmp, MAX_PATH + 1);
	return Path(tmp);
}

Path::Path(const String &path) : isDirectory(false) {
	parseStr(path);
	simplify();
}

Path::Path() : isDirectory(false) {}

void Path::parseStr(const String &str) {
	nat numPaths = std::count(str.begin(), str.end(), '\\');
	numPaths += std::count(str.begin(), str.end(), '/');
	parts.reserve(numPaths + 1);

	nat startAt = 0;
	for (nat i = 0; i < str.size(); i++) {
		if (str[i] == '\\' || str[i] == '/') {
			if (i > startAt) {
				parts.push_back(str.substr(startAt, i - startAt));
			}
			startAt = i + 1;
		}
	}

	if (str.size() > startAt) {
		parts.push_back(str.substr(startAt));
		isDirectory = false;
	} else {
		isDirectory = true;
	}
}

void Path::simplify() {
	vector<String>::iterator i = parts.begin();
	while (i != parts.end()) {
		if (*i == L".") {
			// Safely removed.
			i = parts.erase(i);
		} else if (*i == L".." && i != parts.begin() && *(i - 1) != L"..") {
			// Remove previous path.
			i = parts.erase(--i);
			i = parts.erase(i);
		} else {
			// Nothing to do, continue.
			++i;
		}
	}
}

String Path::toS() const {
	// On Windows, we do not need to know if the path is absolute or relative.
	String result = join(parts, L"\\");
	if (parts.size() == 0)
		result = L".";
	if (isDirectory)
		result += L"\\";
	return result;
}

std::wostream &operator <<(std::wostream &to, const Path &path) {
	for (nat i = 0; i < path.parts.size(); i++) {
		if (i > 0) to << L"\\";
		to << path.parts[i];
	}
	if (path.parts.size() == 0)
		to << L".";
	if (path.isDir())
		to << L"\\";
	return to;
}

bool Path::operator ==(const Path &o) const {
	if (isDirectory != o.isDirectory)
		return false;
	if (parts.size() != o.parts.size())
		return false;

	for (nat i = 0; i < parts.size(); i++)
		if (parts[i] != o.parts[i])
			return false;
	return true;
}

bool Path::operator <(const Path &o) const {
	if (isDirectory != o.isDirectory && parts == o.parts)
		return o.isDirectory;
	return parts < o.parts;
}

bool Path::operator >(const Path &o) const {
	return o < *this;
}


Path Path::operator +(const Path &other) const {
	Path result(*this);
	result += other;
	return result;
}

Path &Path::operator +=(const Path &other) {
	assert(!other.isAbsolute());
	isDirectory = other.isDirectory;
	parts.insert(parts.end(), other.parts.begin(), other.parts.end());
	simplify();
	return *this;
}

Path Path::operator +(const String &name) const {
	Path result(*this);
	result += name;
	return result;
}

Path &Path::operator +=(const String &name) {
	parts.push_back(name);
	isDirectory = false;
	return *this;
}

Path Path::parent() const {
	Path result(*this);
	result.parts.pop_back();
	result.isDirectory = true;
	return result;
}

String Path::title() const {
	return parts.back();
}

String Path::titleNoExt() const {
	String t = title();
	nat last = t.rfind('.');
	return t.substr(0, last);
}

bool Path::hasExt(const String &ext) const {
	String t = title();
	return t.endsWith(L"." + ext);
}

String Path::ext() const {
	String t = title();
	nat p = t.rfind('.');
	if (p == String::npos)
		return L"";
	else
		return t.substr(p + 1);
}

bool Path::isDir() const {
	return isDirectory;
}

bool Path::isAbsolute() const {
	if (parts.size() == 0) return false;
	const String &first = parts.front();
	if (first.size() < 2) return false;
	return first[1] == L':';
}

bool Path::isEmpty() const {
	return parts.size() == 0;
}

Path Path::makeRelative(const Path &to) const {
	Path result;
	result.isDirectory = isDirectory;

	bool equal = true;
	nat consumed = 0;

	for (nat i = 0; i < to.parts.size(); i++) {
		if (!equal) {
			result.parts.push_back(L"..");
		} else if (i >= parts.size()) {
			result.parts.push_back(L"..");
			equal = false;
		} else if (to.parts[i].compareNoCase(parts[i]) != 0) {
			// } else if (to.parts[i] != parts[i]) {
			result.parts.push_back(L"..");
			equal = false;
		} else {
			consumed++;
		}
	}

	for (nat i = consumed; i < parts.size(); i++) {
		result.parts.push_back(parts[i]);
	}

	return result;
}

Path Path::makeAbsolute(const Path &to) const {
	if (isAbsolute())
		return to;
	else
		return to + *this;
}

bool Path::exists() const {
	return PathFileExists(toS().c_str()) == TRUE;
}

void Path::deleteFile() const {
	DeleteFile(toS().c_str());
}

vector<Path> Path::children() const {
	vector<Path> result;

	String searchStr = toS();
	if (!isDir())
		searchStr += L"\\";
	searchStr += L"*";

	WIN32_FIND_DATA findData;
	HANDLE h = FindFirstFile(searchStr.c_str(), &findData);
	if (h == INVALID_HANDLE_VALUE)
		return result;

	do {
		if (wcscmp(findData.cFileName, L"..") != 0 && wcscmp(findData.cFileName, L".") != 0) {
			result.push_back(*this + findData.cFileName);
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				result.back().makeDir();
		}
	} while (FindNextFile(h, &findData));

	FindClose(h);

	return result;
}

Timestamp fromFileTime(FILETIME ft);

Timestamp Path::mTime() const {
	WIN32_FIND_DATA d;
	HANDLE h = FindFirstFile(toS().c_str(), &d);
	if (h == INVALID_HANDLE_VALUE)
		return Timestamp(0);
	FindClose(h);

	return fromFileTime(d.ftLastWriteTime);
}

Timestamp Path::cTime() const {
	WIN32_FIND_DATA d;
	HANDLE h = FindFirstFile(toS().c_str(), &d);
	if (h == INVALID_HANDLE_VALUE)
		return Timestamp(0);
	FindClose(h);

	return fromFileTime(d.ftCreationTime);
}

void Path::createDir() const {
	if (exists())
		return;

	if (isEmpty())
		return;

	parent().createDir();
	CreateDirectory(toS().c_str(), NULL);
}
