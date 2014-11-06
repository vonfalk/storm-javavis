#include "StdAfx.h"
#include "Stream.h"

Stream::Stream() : hasError(false) {}

Stream::~Stream() {}

nat Stream::read(nat size, void *to) {
	assert(false);
	return 0;
}

void Stream::write(nat size, const void *from) {
	assert(false);
}

bool Stream::more() const {
	if (!valid()) return false;
	return pos() < length();
}


//////////////////////////////////////////////////////////////////////////
// Read formatted data.
//////////////////////////////////////////////////////////////////////////

// Helper for simple primitive types and structs.
template <class T>
inline T readPrimitive(Stream *from, bool &error) {
	T t = 0;
	if (from->read(sizeof(T), &t) != sizeof(T)) error = true;
	return t;
}

bool Stream::readBool() {
	return read<char>() != 0;
}

char Stream::readChar() {
	return readPrimitive<char>(this, hasError);
}

byte Stream::readByte() {
	return readPrimitive<byte>(this, hasError);
}

nat Stream::readNat() {
	return readPrimitive<nat>(this, hasError);
}

int Stream::readInt() {
	return readPrimitive<int>(this, hasError);
}

nat64 Stream::readNat64() {
	return readPrimitive<nat64>(this, hasError);
}

int64 Stream::readInt64() {
	return readPrimitive<int64>(this, hasError);
}

float Stream::readFloat() {
	return readPrimitive<float>(this, hasError);
}

double Stream::readDouble() {
	return readPrimitive<double>(this, hasError);
}

String Stream::readString() {
	nat length = read<nat>();
	if (error()) return L"";

	String result(length, L' ');
	nat sz = read(length * sizeof(wchar_t), &result[0]);
	if (sz < length * sizeof(wchar_t)) return L"";

	return result;
}

bool Stream::error() const {
	return hasError;
}

void Stream::clearError() {
	hasError = false;
}

//////////////////////////////////////////////////////////////////////////
// Write formatted data
//////////////////////////////////////////////////////////////////////////
	
void Stream::writeChar(char ch) {
	write(sizeof(ch), &ch);
}

void Stream::writeByte(byte b) {
	write(sizeof(b), &b);
}

void Stream::writeBool(bool b) {
	writeChar(b ? 1 : 0);
}

void Stream::writeNat(nat n) {
	write(sizeof(n), &n);
}

void Stream::writeInt(int n) {
	write(sizeof(n), &n);
}

void Stream::writeNat64(nat64 n) {
	write(sizeof(n), &n);
}

void Stream::writeInt64(int64 n) {
	write(sizeof(n), &n);
}

void Stream::writeDouble(double n) {
	write(sizeof(n), &n);
}

void Stream::writeFloat(float n) {
	write(sizeof(n), &n);
}

void Stream::writeString(const String &s) {
	writeNat(s.size());
	write(sizeof(wchar_t) * s.size(), s.c_str());
}

void Stream::write(Stream *stream) {
	nat64 remaining = stream->length();
	stream->seek(0);

	byte *chunkData = new byte[copySize];

	while (remaining > 0) {
		nat copied = stream->read(copySize, chunkData);
		write(copied, chunkData);
		remaining -= copied;
	}

	delete []chunkData;
}

