#include "stdafx.h"
#include "FileStream.h"
#include "TextReader.h"

FileStream::FileStream(const Path &filename, Mode mode) : mode(mode), name(filename) {
	openFile(filename.toS(), mode);

	bufferStart = 0;
	bufferPos = 0;
	bufferFill = 0;
	buffer = new byte[bufferSize];
	bufferDirty = false;
}

void FileStream::openFile(const String &name, Mode mode) {
	DWORD access = 0;
	if (mode & mRead) access |= GENERIC_READ;
	if (mode & mWrite) access |= GENERIC_WRITE;

	DWORD shareMode = 0;
	if (mode & mRead) shareMode |= FILE_SHARE_READ;

	DWORD createMode = 0;
	if (mode & mRead) createMode = OPEN_EXISTING;
	if (mode & mWrite) createMode = CREATE_ALWAYS;

	file = CreateFile(name.c_str(), access, shareMode, NULL, createMode, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file != INVALID_HANDLE_VALUE) {

		LARGE_INTEGER size;
		GetFileSizeEx(file, &size);
		fileSize = size.QuadPart;
	}
}

void FileStream::flush() {
	// Nothing to write?
	if (!bufferDirty)
		return;
	bufferDirty = false;
	if (bufferPos == 0)
		return;

	if (mode & mWrite) {
		rawSeek(bufferStart);
		DWORD written;
		if (WriteFile(file, buffer, DWORD(bufferPos), &written, NULL)) {
			hasError |= written != bufferPos;
		} else {
			hasError = true;
		}
		bufferStart += bufferPos;
		fileSize = max(fileSize, bufferStart);
		bufferPos = 0;
		bufferFill = 0;
	}
}

bool FileStream::fillBuffer() {
	if (bufferDirty)
		flush();

	bufferStart = pos();
	bufferPos = 0;

	rawSeek(bufferStart);
	DWORD read;
	ReadFile(file, buffer, DWORD(bufferSize), &read, NULL);
	bufferFill = read;

	return bufferFill > 0;
}

FileStream::~FileStream() {
	close();
	delete []buffer;
}

Stream *FileStream::clone() const {
	assert(mode & mRead);
	return new FileStream(name, mode);
}

void FileStream::close() {
	if (valid()) {
		flush();
		CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
	}
}

bool FileStream::valid() const {
	return file != INVALID_HANDLE_VALUE;
}

nat FileStream::read(nat size, void *to) {
	byte *dest = (byte *)to;
	nat read = 0;
	while (read < size) {
		if (bufferPos == bufferFill)
			if (!fillBuffer())
				break;

		nat len = min(bufferFill - bufferPos, size - read);
		memcpy(dest + read, buffer + bufferPos, len);
		bufferPos += len;
		read += len;
	}

	return read;
}

void FileStream::write(nat size, const void *from) {
	const byte *src = (const byte *)from;
	nat pos = 0;
	while (pos < size) {
		if (bufferPos == bufferSize)
			flush();
		nat len = min(bufferSize - bufferPos, size - pos);
		memcpy(buffer + bufferPos, src + pos, len);
		pos += len;
		bufferPos += len;
		bufferDirty = true;
	}
}

nat64 FileStream::pos() const {
	return bufferStart + bufferPos;
}

nat64 FileStream::rawPos() const {
	LARGE_INTEGER result, zero;
	zero.QuadPart = 0;
	SetFilePointerEx(file, zero, &result, FILE_CURRENT);
	return result.QuadPart;
}

nat64 FileStream::length() const {
	return fileSize;
}

void FileStream::seek(nat64 to) {
	flush();
	bufferPos = 0;
	bufferStart = to;
	bufferFill = 0;
}

void FileStream::rawSeek(nat64 to) {
	LARGE_INTEGER distance;
	distance.QuadPart = to;
	SetFilePointerEx(file, distance, NULL, FILE_BEGIN);
}


/**
 * Read entire text file.
 */
String readTextFile(const Path &file) {
	String r;
	TextReader *reader = null;
	try {
		reader = TextReader::create(new FileStream(file, Stream::mRead));
		r = reader->getAll();
		delete reader;
	} catch (...) {
		delete reader;
		throw;
	}
	return r;
}
