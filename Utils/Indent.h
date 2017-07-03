#pragma once

#include <iostream>

/**
 * Adds one layer of indentation to the stream specified in the constructor.
 * This class can be nested!
 */
class Indent : public std::basic_streambuf<wchar_t> {
public:
	// Attach to 'stream' and make 'stream' one step more indented.
	Indent(std::wostream &stream);
	~Indent();

protected:

	// Called to flush buffers.
	virtual int sync();

	// Called when the buffer is full.
	virtual int_type overflow(int_type c);

private:
	// Stream we're attached to.
	std::wostream &owner;

	// The original stream buffer, the one we'll write to.
	std::basic_streambuf<wchar_t> *to;

	// Currently at the start of the line?
	bool startOfLine;

	// Buffer size.
	static const nat bufferSize = 32;

	// Buffer.
	wchar_t buffer[bufferSize];
};
