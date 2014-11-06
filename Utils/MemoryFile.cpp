#include "StdAfx.h"
#include "MemoryFile.h"

MemoryFile::MemoryFile() {
	currPos = 0;
	contents = new Contents();
}

MemoryFile::MemoryFile(Stream *from) {
	currPos = 0;
	contents = new Contents(from);
}

MemoryFile::MemoryFile(const MemoryFile &other) {
	currPos = other.currPos;
	contents = other.contents;
}

MemoryFile &MemoryFile::operator =(const MemoryFile &other) {
	currPos = other.currPos;
	contents = other.contents;

	return *this;
}

MemoryFile::~MemoryFile() {}

Stream *MemoryFile::clone() const {
	return new MemoryFile(*this);
}

bool MemoryFile::valid() const {
	return true;
}

nat MemoryFile::read(nat size, void *buffer) {
	ReadWrite::Reader r(contents->rwLock);

	nat maxRead = size;
	if (maxRead > contents->data.size() - currPos) {
		maxRead = nat(contents->data.size() - currPos);
	}

	// Wait for the loading to progress.
	while (currPos + maxRead > contents->readUntil) Sleep(0);

	if (maxRead == 0) return 0;

	assert(size_t(currPos) == currPos);
	memcpy(buffer, &contents->data[size_t(currPos)], maxRead);
	currPos += maxRead;
	return maxRead;
}

void MemoryFile::write(nat size, const void *buffer) {
	// Wait until loading from file is finished.
	while (contents->readFrom) Sleep(0);

	ReadWrite::Writer w(contents->rwLock);

	nat64 neededSize = currPos + size;
	if (neededSize > contents->data.size()) {
		assert(size_t(neededSize) == neededSize);
		contents->data.resize(size_t(neededSize));
	}

	assert(size_t(currPos) == currPos);
	memcpy(&contents->data[size_t(currPos)], buffer, size);
	currPos += size;
}

nat64 MemoryFile::length() const {
	return contents->data.size();
}

void MemoryFile::seek(nat64 to) {
	currPos = to;
	limitMax(currPos, nat64(contents->data.size()));

}

nat64 MemoryFile::pos() const {
	return currPos;
}

//////////////////////////////////////////////////////////////////////////
// Contents
//////////////////////////////////////////////////////////////////////////

MemoryFile::Contents::Contents(Stream *from) : readFrom(from), readUntil(0) {
	nat64 size = readFrom->length();
	assert(size_t(size) == size); // If 32-bit this makes sense...
	data.resize(size_t(size));

	copyThread.start(memberFn(this, &MemoryFile::Contents::copyFn));
}

MemoryFile::Contents::Contents() : readFrom(null), readUntil(0) {}

MemoryFile::Contents::~Contents() {
	copyThread.stopWait();

	// Should be done by the tread fn if needed, but just to be sure.
	del(readFrom);
}

void MemoryFile::Contents::copyFn(Thread::Control &c) {
	readUntil = 0;

	while (c.run() && readUntil < data.size()) {
		nat64 toRead = min(chunkSize, data.size() - readUntil);

		nat got = readFrom->read(nat(toRead), &data[size_t(readUntil)]);

		readUntil += got;

		if (!readFrom->valid()) {
			assert(false);
			break;
		}
	}

	del(readFrom);
}


