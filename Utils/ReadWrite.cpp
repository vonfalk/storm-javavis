#include "StdAfx.h"
#include "ReadWrite.h"

ReadWrite::ReadWrite() : writers(1) {}

ReadWrite::~ReadWrite() {
	//Lock as writer to ensure everyone has leaved the session?
}

//Reader

ReadWrite::Reader::Reader(const ReadWrite &rw) : rw(rw) {
	Lock::L l(rw.readersLock);
	if (rw.readers++ == 0) {
		//First reader. Wait for writers
		rw.writers.down();
	}
}

ReadWrite::Reader::~Reader() {
	Lock::L l(rw.readersLock);
	if (--rw.readers == 0) {
		//Last reader. Allow writers.
		rw.writers.up();
	}
}

//Writer

ReadWrite::Writer::Writer(ReadWrite &rw) : rw(rw) {
	rw.writers.down();
}

ReadWrite::Writer::~Writer() {
	rw.writers.up();
}

