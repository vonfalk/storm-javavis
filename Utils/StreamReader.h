#pragma once
#include "StreamDescriptor.h"

#include <vector>

namespace util {

	class StreamFile : public CFile {
	public:
		StreamFile(const CFile *oldFile, ULONGLONG offset, ULONGLONG length, const String &name);
		virtual ~StreamFile();

		virtual ULONGLONG GetPosition() const;
		virtual CString GetFileName() const;
		virtual CString GetFileTitle() const;
		virtual CString GetFilePath() const;

		virtual CFile* Duplicate() const;

		virtual ULONGLONG Seek(LONGLONG lOff, UINT nFrom);
		virtual void SetLength(ULONGLONG dwNewLen);
		virtual ULONGLONG GetLength() const;

		virtual UINT Read(void* lpBuf, UINT nCount);
	private:
		ULONGLONG offset, length;
		String filename;
	};

	class StreamReader {
	public:
		//Throws LoadError if failed to open the file.
		StreamReader(const String &filename);
		StreamReader(CFile *f, bool deleteFile = true);
		virtual ~StreamReader(void);

		StreamContents *extract(const String &filename);
		bool extractAll(Streams *to);
		bool open(const String &filename);
		CFile *openAsFile(const String &filename);
		bool seek(LONGLONG offset, bool relative = false);
		UINT read(void *dest, UINT size);
		int readInt();
		nat readNat();
		CString readCString();
		String readString();
		ULONGLONG readLong();
		INT64 readInt64();
		float readFloat();
		bool readBool();
		UINT readUINT();
		CMemFile *readFile(); //Läser från nuv. position
		CMemFile *readFile(const String &from); //Läser en hel "fil" oavsett innehåll.

		bool atEndOfStream();

		bool errorOccured();

		const StreamDescriptor &getStream(int id) { return content[id]; };
		const StreamDescriptor &getStream(const String &name);
		int getStreams() { return int(content.size()); };

		CString getFilePath() { return file->GetFilePath(); };
	private:
		bool deleteFile;
		bool error;
		CFile *file;
		vector<StreamDescriptor> content;
		StreamDescriptor *openStream;
		ULONGLONG position;
		StreamDescriptor zeroDescriptor;

		void close();
		void initialize();
	};

};