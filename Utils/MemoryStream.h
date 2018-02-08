#pragma once
#include "Stream.h"

/**
 * Stream in memory.
 */
class MemoryStream : public Stream {
public:
	MemoryStream();
	// Takes a copy of the data.
	MemoryStream(const byte *data, nat size);
	~MemoryStream();

	virtual Stream *clone() const;

	virtual nat read(nat size, void *to);
	virtual void write(nat size, const void *from);
	virtual nat64 pos() const;
	virtual nat64 length() const;
	virtual void seek(nat64 to);
	virtual bool valid() const;

private:
	byte *data;
	nat size;
	nat capacity;
	nat p;

	// Ensure at least 'n' bytes.
	void ensure(nat n);
};
