#include "StdAfx.h"
#include "FileStream.h"
#include "TextReader.h"

FileStream::FileStream(const Path &filename, Mode mode) : mode(mode), name(filename) {
	openFile(filename.toS(), mode);
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

FileStream::~FileStream() {
	close();
}

Stream *FileStream::clone() const {
	assert(mode & mRead);
	return new FileStream(name, mode);
}

void FileStream::close() {
	if (valid()) {
		CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
	}
}

bool FileStream::valid() const {
	return file != INVALID_HANDLE_VALUE;
}

nat FileStream::read(nat size, void *to) {
	DWORD result = 0;
	if (ReadFile(file, to, DWORD(size), &result, NULL)) {
		return result;
	} else {
		hasError = true;
		return 0;
	}
}

void FileStream::write(nat size, const void *from) {
	DWORD result = 0;
	if (WriteFile(file, from, DWORD(size), &result, NULL)) {
		hasError |= result != size;
	} else {
		hasError = true;
	}
}

nat64 FileStream::pos() const {
	LARGE_INTEGER result, zero;
	zero.QuadPart = 0;
	SetFilePointerEx(file, zero, &result, FILE_CURRENT);
	return result.QuadPart;
}

nat64 FileStream::length() const {
	return fileSize;
}

void FileStream::seek(nat64 to) {
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
