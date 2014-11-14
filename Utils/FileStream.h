#pragma once

#include "Path.h"
#include "Stream.h"

// Wrapper around a file handle on windows. However, noting windows-specific is exposed
// by this class, which allows it to be ported to other operating systems.
class FileStream : public Stream {
public:
	FileStream(const Path &path, Mode mode);
	virtual ~FileStream();

	virtual Stream *clone() const;

	void close();

	using Stream::read;
	virtual nat read(nat size, void *to);
	using Stream::write;
	virtual void write(nat size, const void *from);

	virtual nat64 pos() const;
	virtual nat64 length() const;
	virtual void seek(nat64 to);

	virtual bool valid() const;
private:
	Path name;
	HANDLE file;
	Mode mode;
	nat64 fileSize;

	void openFile(const String &name, Mode mode);
};


/**
 * Read an entire text file.
 */
String readTextFile(const Path &file);
