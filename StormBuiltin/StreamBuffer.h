#pragma once
#include "Utils/Stream.h"

/**
 * Buffer output to another stream (eg. FileStream). Implementing minimal
 * possible to support writing output files faster.
 */
class StreamBuffer : public Stream {
public:
	// Create writing to another stream, with buffer size.
	StreamBuffer(Stream *to, nat buffer = 10 * 1024);
	~StreamBuffer();

	virtual Stream *clone() const;
	virtual void write(nat size, const void *from);

	virtual nat64 pos() const;
	virtual nat64 length() const;
	virtual void seek(nat64 to);
	virtual bool valid() const;

	// Flush buffer.
	void flush();

private:
	Stream *to;

	byte *buffer;
	nat bufferSize;
	nat bufferPos;
};
