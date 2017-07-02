#pragma once
#include "Object.h"
#include "Bitmask.h"

// A basic stream class.
// Any object that can follows the following interface can be loaded/saved using this class.
// class Foo {
// public:
//     static Foo load(Stream &from);
//     void save(Stream &to) const;
// }
// Note that even though the type of the parameter to "write" can be deduced, it is the standard
// to write it out, so that innocent changes to datatypes does not break the file structure without
// warnings.
class Stream : NoCopy {
public:
	enum Mode { mNone = 0x0, mRead = 0x1, mWrite = 0x2, mRandom = 0x3 };

	Stream();
	virtual ~Stream();

	// Clone this stream.
	virtual Stream *clone() const = 0;

	// Non-implemented reads and write simply assert.
	virtual nat read(nat size, void *to);
	virtual void write(nat size, const void *from);

	virtual nat64 pos() const = 0;
	virtual nat64 length() const = 0;
	virtual void seek(nat64 to) = 0;
	virtual bool more() const;

	virtual bool valid() const = 0;

	// See if any error has occurred.
	virtual bool error() const;

	// Clear the error flag.
	virtual void clearError();

	// Write an entire stream.
	void write(Stream *from);

protected:
	// Error during stream operation.
	bool hasError;

private:
	// Chunk size when copying streams.
	static const nat copySize = 10 * 1024;
};

BITMASK_OPERATORS(Stream::Mode);
