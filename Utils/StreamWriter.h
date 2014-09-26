#pragma once
#include "StreamDescriptor.h"
#include "MemoryFile.h"
#include <list>

namespace util {

	class StreamWriter {
	public:
		//Throws SaveError on failure.
		StreamWriter(const String &file);
		StreamWriter(CFile *f, bool deleteFile = true);
		~StreamWriter();

		bool open(const String &name);
		bool write(const void *data, UINT size);
		bool write(const String &s);
		inline bool write(const CString &str) { return write(String(str)); }
		bool write(int i);
		bool write(INT64);
		bool write(ULONGLONG v);
		bool writeFile(const String &path); //Sparar på nuv. position
		bool writeFile(const String &path, const String &to); //Sparar som en ny fil.
		bool writeFile(MemoryFile file, const String &to);
		bool write(CFile *f, const String &to); //Sparar som en ny fil.
		bool write(CFile *f); //Sparar på nuv. position
		bool write(float v);
		bool write(bool v);
		bool write(UINT v);
		bool write(StreamContents *sc);

		String getFilePath() { return file->GetFilePath().GetString(); };

	private:
		bool deleteFile;
		std::list<StreamDescriptor> contents;
		StreamDescriptor *openStream;
		CFile *file;

		void close();
	};

};