#pragma once

#include "Object.h"

#include <vector>

namespace util {

	class StreamDescriptor {
	public:
		StreamDescriptor();
		StreamDescriptor(const String &name, ULONGLONG offset);
		StreamDescriptor(const String &name, ULONGLONG offset, ULONGLONG size);
		virtual ~StreamDescriptor();

		String name;
		ULONGLONG length;
		ULONGLONG offset;
	};

	class StreamContents {
	public:
		StreamContents(const String &name, CFile &readFrom, UINT size);
		virtual ~StreamContents();

		String name;
		UINT size;
		byte *data;
	};

	class Streams : NoCopy {
	public:
		Streams();
		virtual ~Streams();

		void add(StreamContents *sc) { data.push_back(sc); };
		StreamContents *get(const String &name);

	private:
		vector<StreamContents *> data;
	};

};