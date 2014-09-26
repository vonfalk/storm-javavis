#pragma once

#include "Object.h"
#include "Lock.h"
#include "Semaphore.h"

namespace util {

	//An implementation of a readers-writers algorithm.
	//Usage: Declare the ReadWrite, then lock by instantiating Reader or Writer.
	class ReadWrite : NoCopy {
	public:
		ReadWrite();
		~ReadWrite();

		//Lock as reader
		class Reader : NoCopy {
		public:
			Reader(const ReadWrite &rw);
			~Reader();
		private:
			const ReadWrite &rw;
		};

		//Lock as writer
		class Writer : NoCopy {
		public:
			//Could be const ReadWrite &rw, but if you need the const version
			//you are probably mis-using this object...
			Writer(ReadWrite &rw);
			~Writer();
		private:
			ReadWrite &rw;
		};
	private:
		mutable nat readers;
		mutable Lock readersLock;
		mutable Semaphore writers;
	};

	//Simplification of locking as reader and writer.
	typedef ReadWrite::Reader Reader;
	typedef ReadWrite::Writer Writer;

}