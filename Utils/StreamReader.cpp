#include "StdAfx.h"
#include "StreamReader.h"

#include "Exception.h"

namespace util {

	StreamReader::StreamReader(const String &filename) {
		deleteFile = true;
		file = new CFile();
		if (file->Open(filename.c_str(), CFile::modeRead | CFile::shareDenyWrite) == FALSE) {
			delete file;
			throw LoadError(L"Failed to open file " + filename);
		}

		initialize();
	}

	StreamReader::StreamReader(CFile *f, bool del) {
		deleteFile = del;
		file = f;

		initialize();
	}

	void StreamReader::initialize() {
		zeroDescriptor = StreamDescriptor(L"", 0, 0);

		StreamDescriptor tmp(L"", 0, file->GetLength());
		position = 0;
		openStream = &tmp;

		//läs in toc
		file->Seek(-int(sizeof(ULONGLONG)), CFile::end);
		ULONGLONG offset = readLong();
		file->Seek(offset, CFile::begin);

		int numEntrys = readInt();

		content.resize(numEntrys, StreamDescriptor());

		for (int i = 0; i < numEntrys; i++) {
			content[i].name = readCString();
			content[i].offset = readLong();
			content[i].length = readLong();
		}

		openStream = null;
		position = 0;
	}

	StreamReader::~StreamReader() {
		if (deleteFile) file->Close();
		if (deleteFile) delete file;
	}

	bool StreamReader::open(const String &filename) {
		position = 0;
		openStream = 0;
		error = false;
		for (nat i = 0; i < content.size(); i++) {
			if (filename == content[i].name) {
				openStream = &content[i];
				file->Seek(openStream->offset, CFile::begin);
				return true;
			}
		}

		openStream = 0;
		return false;
	}

	CFile *StreamReader::openAsFile(const String &filename) {
		for (nat i = 0; i < content.size(); i++) {
			if (filename == content[i].name) {
				StreamDescriptor *sd = &content[i];
				return new StreamFile(file, sd->offset, sd->length, sd->name);
			}
		}

		return 0;
	}

	UINT StreamReader::read(void *dest, UINT size) {
		if (openStream == 0) return 0;

		if (position + size > openStream->length) {
			size = UINT(openStream->length - position);
		}
		
		UINT r = file->Read(dest, size);
		position += r;
		return r;
	}

	int StreamReader::readInt() {
		int ret = 0;
		if (read(&ret, sizeof(int)) < sizeof(int)) error = true;
		return ret;
	}

	nat StreamReader::readNat() {
		nat ret = 0;
		if (read(&ret, sizeof(nat)) < sizeof(nat)) error = true;
		return ret;
	}

	CString StreamReader::readCString() {
		return CString(readString().c_str());
	}

	String StreamReader::readString() {
		nat sz = readNat();
		if (sz == 0) {
			return L"";
		} else if (sz < 0) {
			error = true;
			return L"";
		} else if (error == true) {
			return L"";
		}

		wchar_t *buf = new wchar_t[sz + 1];
		buf[sz] = 0;
		if (read(buf, sizeof(wchar_t) * sz) != sizeof(wchar_t) * sz) {
			delete []buf;
			error = true;
			return L"";
		}

		String ret = buf;
		delete []buf;
		return ret;
	}

	ULONGLONG StreamReader::readLong() {
		ULONGLONG ret = 0;
		if (read(&ret, sizeof(ULONGLONG)) != sizeof(ULONGLONG)) error = true;
		return ret;
	}

	INT64 StreamReader::readInt64() {
		INT64 ret = 0;
		if (read(&ret, sizeof(INT64)) != sizeof(INT64)) error = true;
		return ret;
	}

	float StreamReader::readFloat() {
		float ret = 0;
		if (read(&ret, sizeof(float)) != sizeof(float)) error = true;
		return ret;
	}

	CMemFile *StreamReader::readFile(const String &from) {
		if (!open(from)) return null;

		CMemFile *f = new CMemFile();
		byte *buf = new byte[UINT(openStream->length)];

		UINT data = read(buf, UINT(openStream->length));
		f->Write(buf, data);

		delete []buf;

		f->Seek(0, CFile::begin);

		return f;
	}

	CMemFile *StreamReader::readFile() {
		UINT length = readUINT();
		if (errorOccured()) return null;

		CMemFile *f = new CMemFile();
		byte *buf = new byte[length];

		UINT data = read(buf, length);
		f->Write(buf, data);

		delete []buf;

		f->Seek(0, CFile::begin);

		return f;
	}

	bool StreamReader::errorOccured() {
		if (error) {
			error = false;
			return true;
		}
		return false;
	}

	bool StreamReader::readBool() {
		byte t = 0;
		if (read(&t, 1) != 1) error = true;
		return (t != 0);
	}

	UINT StreamReader::readUINT() {
		UINT ret = 0;
		if (read(&ret, sizeof(UINT)) != sizeof(UINT)) error = true;
		return ret;
	}

	const StreamDescriptor &StreamReader::getStream(const String &name) {
		for (nat i = 0; i < content.size(); i++) {
			if (name == content[i].name) {
				
				return content[i];
			}
		}
		return zeroDescriptor;
	}

	StreamContents *StreamReader::extract(const String &name) {
		if (!open(name)) return 0;

		if (openStream->length != UINT(openStream->length)) return 0;

		StreamContents *sc = new StreamContents(name, *file, UINT(openStream->length));
		if (sc->data == 0) {
			delete sc;
			return 0;
		}
		return sc;
	}

	bool StreamReader::extractAll(Streams *to) {
		bool success = true;
		for (nat i = 0; i < content.size(); i++) {
			StreamContents *t = extract(content[i].name);
			if (t == 0) success = false;
			to->add(t);
		}
		return success;
	}

	bool StreamReader::atEndOfStream() {
		return position >= openStream->length;
	}



	//////////////////////////////////////////////////////////////////////////
	// StreamFile
	//////////////////////////////////////////////////////////////////////////

	StreamFile::StreamFile(const CFile *f, ULONGLONG offset, ULONGLONG length, const String &name) {
		this->offset = offset;
		this->length = length;
		this->filename = name;

		CFile::Open(f->GetFilePath(), CFile::modeRead | CFile::shareDenyWrite);
		CFile::Seek(offset, CFile::begin);
	}

	StreamFile::~StreamFile() {}

	ULONGLONG StreamFile::GetPosition() const {
		return CFile::GetPosition() - offset;
	}

	CString StreamFile::GetFilePath() const {
		return CFile::GetFilePath() + L"|" + CString(filename.c_str());
	}

	CString StreamFile::GetFileName() const {
		return CString(filename.c_str());
	}

	CString StreamFile::GetFileTitle() const {
		return CString(filename.c_str());
	}

	CFile* StreamFile::Duplicate() const {
		return new StreamFile(this, offset, length, filename);
	}

	ULONGLONG StreamFile::Seek(LONGLONG lOff, UINT nFrom) {
		LONGLONG pos = GetPosition();
		switch (nFrom) {
			case CFile::begin:
				pos = LONGLONG(lOff);
				break;
			case CFile::end:
				pos = LONGLONG(length + lOff);
				break;
			case CFile::current:
				pos = LONGLONG(pos + lOff);
				break;
		}
		if (pos > LONGLONG(length)) pos = length;
		if (pos < 0) pos = 0;
		return CFile::Seek(pos + offset,CFile::begin) - offset;
	}

	void StreamFile::SetLength(ULONGLONG dwNewLen) {}

	ULONGLONG StreamFile::GetLength() const {
		return length;
	}

	UINT StreamFile::Read(void* lpBuf, UINT nCount) {
		if (GetPosition() + nCount >= length) {
			nCount = UINT(length - GetPosition());
		}
		return CFile::Read(lpBuf, nCount);
	}

}