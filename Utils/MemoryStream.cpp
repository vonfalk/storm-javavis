#include "stdafx.h"
#include "MemoryStream.h"

MemoryStream::MemoryStream() : data(null), size(0), capacity(0), p(0) {}

MemoryStream::MemoryStream(const byte *data, nat size) : size(size), capacity(size), p(0) {
	this->data = new byte[size];
	memcpy(this->data, data, size);
}

MemoryStream::~MemoryStream() {
	delete[] data;
}

Stream *MemoryStream::clone() const {
	return new MemoryStream(data, size);
}

void MemoryStream::write(nat count, const void *from) {
	ensure(p + count);

	memcpy(data + p, from, count);
	p += count;
	size = max(size, p);
}

nat MemoryStream::read(nat count, void *to) {
	count = min(count, size - p);

	memcpy(to, data + p, count);
	p += count;
	return count;
}

nat64 MemoryStream::pos() const {
	return p;
}

nat64 MemoryStream::length() const {
	return size;
}

void MemoryStream::seek(nat64 to) {
	p = nat(min(nat64(size), to));
}

bool MemoryStream::valid() const {
	return true;
}

void MemoryStream::ensure(nat n) {
	if (capacity >= n)
		return;

	capacity = max(128u, max(capacity*2, n));
	byte *old = data;
	data = new byte[capacity];

	if (old) {
		memcpy(data, old, size);
		delete []old;
	}
}
