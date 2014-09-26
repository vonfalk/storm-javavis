#include "StdAfx.h"
#include "StreamDescriptor.h"

namespace util {

	StreamDescriptor::StreamDescriptor() {}

	StreamDescriptor::StreamDescriptor(const String &name, ULONGLONG offset) {
		this->name = name;
		this->offset = offset;
		this->length = 0;
	}

	StreamDescriptor::StreamDescriptor(const String &name, ULONGLONG offset, ULONGLONG size) {
		this->name = name;
		this->offset = offset;
		this->length = size;
	}

	StreamDescriptor::~StreamDescriptor() {}

	//////////////////////////////////////////////////////////////////////////
	// StreamContents
	//////////////////////////////////////////////////////////////////////////

	StreamContents::StreamContents(const String &name, CFile &readFrom, UINT size) {
		this->size = size;
		this->name = name;

		data = new byte[size];
		if (readFrom.Read(data, size) != size) {
			delete []data;
			data = 0;
		}
	}

	StreamContents::~StreamContents() {
		if (data != 0) delete []data;
	}

	//////////////////////////////////////////////////////////////////////////
	// Streams
	//////////////////////////////////////////////////////////////////////////

	Streams::Streams() {}

	Streams::~Streams() {
		for (nat i = 0; i < data.size(); i++) {
			if (data[i] != 0) delete data[i];
		}
	}

	StreamContents *Streams::get(const String &name) {
		for (nat i = 0; i < data.size(); i++) {
			if (data[i] != 0) {
				if (data[i]->name.compareNoCase(name) == 0) {
					return data[i];
				}
			}
		}

		return 0;
	}
}