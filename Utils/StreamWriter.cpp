#include "StdAfx.h"
#include "StreamWriter.h"
#include "StreamReader.h"

#include "Exception.h"

namespace util {

	StreamWriter::StreamWriter(const String &file) {
		deleteFile = true;
		this->file = new CFile();
		if (this->file->Open(file.c_str(), CFile::modeWrite | CFile::shareDenyWrite | CFile::modeCreate) == FALSE) {
			delete this->file;
			throw SaveError(L"Failed to open " + file);
		}
		openStream = null;
	}

	StreamWriter::StreamWriter(CFile *f, bool del) {
		deleteFile = del;
		file = f;
		openStream = null;
	}

	StreamWriter::~StreamWriter() {
		close();

		//Skriv ut TOC
		openStream = (StreamDescriptor *)1;	//För att write() ska fungera!
		ULONGLONG beginPos = file->GetPosition();
		
		write(contents.size());
		for (std::list<StreamDescriptor>::iterator i = contents.begin(); i != contents.end(); i++) {
			write(i->name);
			write(i->offset);
			write(i->length);
		}

		//Skriv vart toc finns!
		write(beginPos);
		if (deleteFile) file->Close();
		if (deleteFile) delete file;
	}

	bool StreamWriter::open(const String &name) {
		close();

		for (std::list<StreamDescriptor>::iterator i = contents.begin(); i != contents.end(); i++) {
			if (name == i->name) return false;
		}

		contents.push_back(StreamDescriptor(name, file->GetPosition()));
		openStream = &contents.back();

		return true;
	}

	void StreamWriter::close() {
		if (openStream != null) {
			openStream->length = file->GetLength() - openStream->offset;
		}
		openStream = null;
	}

	bool StreamWriter::write(const void *data, UINT size) {
		if (openStream == 0) return false;

		file->Write(data, size);
		return true;
	}

	bool StreamWriter::write(int i) {
		return write(&i, sizeof(i));
	}

	bool StreamWriter::write(const String &s) {
		bool ok = write(s.size());
		if (s.size() > 0)
			ok |= write(s.c_str(), s.size() * sizeof(wchar_t));
		return ok;
	}

	bool StreamWriter::write(ULONGLONG v) {
		return write(&v, sizeof(ULONGLONG));
	}

	bool StreamWriter::write(INT64 i) {
		return write(&i, sizeof(INT64));
	}

	bool StreamWriter::write(float v)  {
		return write(&v, sizeof(float));
	}

	bool StreamWriter::writeFile(const String &path, const String &to) {
		CFile *src = 0;
		if (path.find('|') != String::npos) {
			CFile *tmp = new CFile();
			if (tmp->Open(path.left(path.find('|')).c_str(), CFile::modeRead | CFile::shareDenyWrite) == FALSE) {
				delete tmp;
				return false;
			}
			StreamReader input(tmp);
			src = input.openAsFile(path.mid(path.find('|') + 1));
		} else {
			src = new CFile();
			if (src->Open(path.c_str(), CFile::modeRead | CFile::shareDenyWrite) == FALSE) {
				delete src;
				return false;
			}
		}

		if (src == 0) return false;

		bool ret = write(src, to);

		src->Close();
		delete src;

		return ret;
	}

	bool StreamWriter::writeFile(const String &path) {
		CFile *src = 0;
		if (path.find('|') != String::npos) {
			CFile *tmp = new CFile();
			if (tmp->Open(path.left(path.find('|')).c_str(), CFile::modeRead | CFile::shareDenyWrite) == FALSE) {
				delete tmp;
				return false;
			}
			StreamReader input(tmp);
			src = input.openAsFile(path.mid(path.find('|') + 1));
		} else {
			src = new CFile();
			if (src->Open(path.c_str(), CFile::modeRead | CFile::shareDenyWrite) == FALSE) {
				delete src;
				return false;
			}
		}

		if (src == 0) return false;

		bool ret = write(src);

		src->Close();
		delete src;

		return ret;
	}

	bool StreamWriter::writeFile(util::MemoryFile file, const String &to) {
		const nat bufferSize = 10 * 1024;
		byte buffer[bufferSize];
		
		if (!open(to)) return false;
		
		file.seek(0);
		while (file.more()) {
			nat filled = file.read(bufferSize, buffer);
			write(buffer, filled);
		}
		return true;
	}

	bool StreamWriter::write(CFile *src) {

		UINT length = UINT(src->GetLength());
		write(length);
		src->Seek(0, CFile::begin);

		UINT bufSz = 1024 * 100;
		byte *buffer = new byte[bufSz];

		ULONGLONG bytesLeft = src->GetLength();

		while (bytesLeft > 0) {

			if (bytesLeft < bufSz) {
				if ((ULONGLONG)src->Read(buffer, (UINT)bytesLeft) < bytesLeft) break;
				if (!write(buffer, (UINT)bytesLeft)) break;
				bytesLeft = 0;
			} else {
				if ((ULONGLONG)src->Read(buffer, bufSz) < bufSz) break;
				if (!write(buffer, bufSz)) break;
				bytesLeft -= bufSz;
			}
		}

		delete []buffer;

		return bytesLeft == 0;
	}

	bool StreamWriter::write(CFile *src, const String &to) {

		if (!open(to)) return false;

		src->Seek(0, CFile::begin);

		UINT bufSz = 1024 * 100;
		byte *buffer = new byte[bufSz];

		ULONGLONG bytesLeft = src->GetLength();

		while (bytesLeft > 0) {

			if (bytesLeft < bufSz) {
				if ((ULONGLONG)src->Read(buffer, (UINT)bytesLeft) < bytesLeft) break;
				if (!write(buffer, (UINT)bytesLeft)) break;
				bytesLeft = 0;
			} else {
				if ((ULONGLONG)src->Read(buffer, bufSz) < bufSz) break;
				if (!write(buffer, bufSz)) break;
				bytesLeft -= bufSz;
			}
		}

		delete []buffer;

		return bytesLeft == 0;
	}

	bool StreamWriter::write(bool v) {
		byte t = (v ? 1 : 0);
		return write(&t, 1);
	}

	bool StreamWriter::write(UINT v) {
		return write(&v, sizeof(UINT));
	}

	bool StreamWriter::write(StreamContents *sc) {
		return write(sc->data, sc->size);
	}

}