#pragma once

#include "Stream.h"
#include "Path.h"
#include <vector>

namespace util {
	// Specialized stream for these classes.
	class CollStream;

	// Header for each of the sub-streams.
	struct StreamCollHeader;

	// Treat a stream as a collection of named streams, assumed to be in a specific format.
	// Only one single stream can be read from at once. Once a new stream is open(), the old ones
	// will stop working. They will also stop working when this class is destroyed.
	class StreamCollReader : NoCopy {
	public:
		// Create from a stream.
		StreamCollReader(Stream *src);

		// Create from a file (using FileStream)
		StreamCollReader(const Path &file);

		~StreamCollReader();

		// Open a stream for reading, allowing the use of this stream for as long
		// as another stream is not opened, and this object is not destroyed.
		// The returned object is completely managed by this object and does therefore not
		// need to be delete'd. Returns null on failure.
		Stream *open(const String &name);

		// Return if the underlying stream is valid or not, and if the header was correctly read.
		bool valid();
	private:
		typedef vector<StreamCollHeader> Headers;
		Headers headers;

		Stream *from;

		bool isValid;

		// The managed CollStream object.
		CollStream *outputStream;

		// Read the header.
		void readHeader();

		// Open a stream by its id.
		Stream *openId(nat id);

		// Close the output stream if needed.
		void closeOutput();
	};

	// The corresponding writer class for a stream collection. This always creates a new file, so each and
	// every stream needs to be re-created.
	class StreamCollWriter : NoCopy {
	public:
		// Create from a stream.
		StreamCollWriter(Stream *src);

		// Create from a file (using FileStream)
		StreamCollWriter(const Path &file);

		~StreamCollWriter();

		// Open a stream for writing, allowing the use of this stream for as long as another stream
		// is not opened, and this object is not destroyed.
		// The returned object is completely managed by this object and does therefore not need to be
		// delete'd.
		Stream *open(const String &name);

		// Add an entire stream to this. This will use open().
		bool addStream(Stream *from, const String &name);

		// Any error during the saving?
		bool anyError();

	private:
		typedef vector<StreamCollHeader> Headers;
		Headers headers;

		Stream *to;

		bool hasAnyError;

		// The managed CollStream object.
		CollStream *inputStream;

		// Save the header in the file.
		void saveHeader();

		// Close the input stream.
		void closeInput();
	};
}