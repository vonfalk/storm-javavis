#include "stdafx.h"
#include "StreamBuffer.h"

StreamBuffer::StreamBuffer(Stream *to, nat buffer) : to(to), bufferSize(buffer), bufferPos(0) {
	this->buffer = new byte[bufferSize];
}

StreamBuffer::~StreamBuffer() {
	flush();
	delete []buffer;
	delete to;
}

nat64 StreamBuffer::pos() const {
	return to->pos() + bufferPos;
}

nat64 StreamBuffer::length() const {
	return to->length() + bufferPos;
}

void StreamBuffer::seek(nat64 to) {
	assert(false, L"Not supported.");
}

bool StreamBuffer::valid() const {
	return to->valid();
}

void StreamBuffer::flush() {
	if (bufferPos == 0)
		return;

	to->write(bufferPos, buffer);
	bufferPos = 0;
}

Stream *StreamBuffer::clone() const {
	assert(false, L"Not implemented!");
}

void StreamBuffer::write(nat size, const void *from) {
	byte *src = (byte *)from;
	nat at = 0;
	while (at < size) {
		nat copy = min(bufferSize - bufferPos, size - at);
		memcpy(buffer + bufferPos, src + at, copy);
		bufferPos += copy;
		at += copy;

		if (bufferPos == bufferSize)
			flush();
	}
}
