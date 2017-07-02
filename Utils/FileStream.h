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

	// Flush buffer.
	void flush();

private:
	static const nat bufferSize = 10 * 1024;

	Path name;
	Mode mode;
	nat64 fileSize;

#if defined(WINDOWS)
	HANDLE file;
#elif defined(POSIX)
	int file;
#endif

	// The buffer is always flushed when seeking. When reading data,
	// the buffer is pre-loaded until bufferFill. When writing, the buffer may
	// contain dirty data until bufferPos.
	// The seek-pointer of the underlying file will always point to the beginning of this buffer.
	nat64 bufferStart;
	byte *buffer;
	nat bufferPos;
	nat bufferFill;
	bool bufferDirty;

	// Fill the buffer from the file. Returns false if eof.
	bool fillBuffer();

	void openFile(const String &name, Mode mode);

	// Raw seek, does not affect the buffer.
	void rawSeek(nat64 to);
	nat64 rawPos() const;
};


/**
 * Read an entire text file.
 */
String readTextFile(const Path &file);
