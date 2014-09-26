#include "StdAfx.h"
#include "Includes.h"

#include "FileStream.h"
#include "Path.h"

#include <Shlwapi.h>

namespace util {

	bool NoIncludes::containsStream(const String &title) {
		return false;
	}

	Stream *NoIncludes::openStream(const String &title, Stream::Mode mode) {
		return null;
	}


	PathIncludes::PathIncludes(const Path &path) : path(path) {}

	bool PathIncludes::containsStream(const String &title) {
		return (path + title).exists();
	}

	Stream *PathIncludes::openStream(const String &title, Stream::Mode mode) {
		if (containsStream(title)) {
			return new FileStream(path + title, mode);
		} else {
			return null;
		}
	}
}