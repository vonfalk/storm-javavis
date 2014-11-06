#pragma once

#include "Stream.h"
#include "Path.h"

// Class used together with a stream to get access to other files.
// Think of this as passing in a reference to a directory together along
// with a stream to compile a *.cpp-file. When the compiler then comes across
// a #include statement, it then queries this interface for Streams representing
// the #included files.
// This is an abstract base class with a few common specializations later on.
class Includes : NoCopy {
public:
	// Check if this Include object contains a stream named "title".
	virtual bool containsStream(const String &title) = 0;

	// Open the stream named "title". This should succeed when "containsStream"
	// returns true. The returned pointer must be freed by the caller.
	virtual Stream *openStream(const String &title, Stream::Mode mode) = 0;
};

// An empty include stream.
class NoIncludes : public Includes {
public:
	virtual bool containsStream(const String &title);
	virtual Stream *openStream(const String &title, Stream::Mode mode);
};

// An include stream associated with a specific directory. Intended to be used together with the
// FileStream class.
class PathIncludes : public Includes {
public:
	// Create this object associated with a specific path. If !isDirectory(path), path = getDirectory(path)
	PathIncludes(const Path &path);

	virtual bool containsStream(const String &title);
	virtual Stream *openStream(const String &title, Stream::Mode mode);

private:
	Path path;
};
