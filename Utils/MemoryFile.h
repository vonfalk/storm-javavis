#pragma once

#include "Stream.h"
#include "Thread.h"
#include "AutoPtr.h"
#include "ReadWrite.h"

namespace util {

	// Copies a file into memory and allows several readers from the file.
	// This file is backed by a shared content, which is removed when no more instances
	// of this class is available.
	// This stream also supports writing of a file.
	class MemoryFile : public Stream {
	public:
		// The file to clone (async copy).
		explicit MemoryFile(Stream *source);

		// Create an empty file prepared for being written to.
		explicit MemoryFile();

		MemoryFile(const MemoryFile &other);
		MemoryFile &operator =(const MemoryFile &other);
		~MemoryFile();

		// Read operations past the end of the stream will block until the data has been read.
		virtual nat read(nat size, void *buffer);

		// Write operations while buffering the content will block until the reading is done.
		virtual void write(nat size, const void *buffer);


		virtual nat64 length() const;
		virtual nat64 pos() const;
		virtual void seek(nat64 to);

		virtual Stream *clone() const;

		virtual bool valid() const;
	private:

		class Contents : public RefCount {
		public:
			Contents();
			Contents(Stream *source);
			~Contents();

			static const nat64 chunkSize = 16 * 1024;

			// The data in this file. Always sized to fit the actual data.
			vector<byte> data;

			// How far has the file been loaded? Invalid if "readFrom" == null
			nat64 readUntil;

			// Reading from a file. Will be set to null when/if there is no more data to load.
			Stream *readFrom;

			// The thread in charge of copying data (if any).
			Thread copyThread;

			// Reader-writer lock for the stream data. Not used by the copy thread to enhanche performance.
			ReadWrite rwLock;

			void copyFn(Thread::Control &c);
		};

		AutoPtr<Contents> contents;
		nat64 currPos;
	};

}
