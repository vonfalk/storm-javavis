#include "stdafx.h"
#include "StreamCollection.h"

#include "FileStream.h"

namespace util {

	class CollStream : public Stream {
	public:
		CollStream(Stream *src, bool ownsStream, nat64 offset, nat64 length) : src(src), l(length), offset(offset), owner(ownsStream) {
			src->seek(offset);
		}

		~CollStream() {
			if (owner) del(src);
		}

		virtual Stream *clone() const {
			return new CollStream(src->clone(), true, offset, l);
		}

		virtual nat read(nat size, void *to) {
			nat64 remaining = l - pos();
			if (size > remaining) size = nat(remaining);
			return src->read(size, to);
		}

		virtual void write(nat size, const void *from) {
			return src->write(size, from);
		}

		virtual nat64 pos() const {
			return src->pos() - offset;
		}

		virtual nat64 length() const {
			return l;
		}

		virtual void seek(nat64 to) {
			src->seek(offset + to);
		}

		virtual bool valid() const {
			return src->valid();
		}

		virtual bool error() const {
			return Stream::error() | src->error();
		}

		virtual void clearError() {
			src->clearError();
			Stream::clearError();
		}

	private:
		Stream *src;
		nat64 l, offset;
		bool owner;
	};

	struct StreamCollHeader {
		String name;
		nat64 offset;
		nat64 length;
	};

	//////////////////////////////////////////////////////////////////////////
	// StreamCollReader class
	//////////////////////////////////////////////////////////////////////////
	StreamCollReader::StreamCollReader(Stream *src) : from(src) {
		readHeader();
	}

	StreamCollReader::StreamCollReader(const Path &file) : from(new FileStream(file, Stream::mRead)) {
		readHeader();
	}

	StreamCollReader::~StreamCollReader() {
		closeOutput();
		del(from);
	}

	void StreamCollReader::readHeader() {
		outputStream = null;
		isValid = false;
		if (!from->valid()) return;

		// The offset of the header is stored as the last nat64 in the file.
		from->seek(from->length() - sizeof(nat64));
		nat64 offset = from->read<nat64>();
		if (from->error()) return;

		from->seek(nat(offset));
		nat numEntries = from->read<nat>();
		if (from->error()) return;
		headers.reserve(numEntries);

		for (nat i = 0; i < numEntries; i++) {
			StreamCollHeader sch;
			sch.name = from->read<String>();
			sch.offset = from->read<nat64>();
			sch.length = from->read<nat64>();

			if (from->error()) return;

			headers.push_back(sch);
		}

		isValid = true;
	}

	bool StreamCollReader::valid() {
		return isValid && from->valid();
	}

	Stream *StreamCollReader::open(const String &name) {
		for (nat i = 0; i < headers.size(); i++) {
			if (headers[i].name == name) return openId(i);
		}
		return null;
	}

	Stream *StreamCollReader::openId(nat id) {
		closeOutput();

		StreamCollHeader &header = headers[id];
		outputStream = new CollStream(from, false, header.offset, header.length);
		return outputStream;
	}

	void StreamCollReader::closeOutput() {
		del(outputStream);
	}

	//////////////////////////////////////////////////////////////////////////
	// StreamCollWriter
	//////////////////////////////////////////////////////////////////////////
	StreamCollWriter::StreamCollWriter(Stream *src) : to(src), inputStream(null), hasAnyError(false) {}

	StreamCollWriter::StreamCollWriter(const Path &file) : inputStream(null), hasAnyError(false) {
		to = new FileStream(file, Stream::mWrite);
	}

	StreamCollWriter::~StreamCollWriter() {
		closeInput();
		saveHeader();
		del(to);
	}

	bool StreamCollWriter::anyError() {
		return hasAnyError;
	}

	Stream *StreamCollWriter::open(const String &name) {
		closeInput();

		for (nat i = 0; i < headers.size(); i++) {
			if (headers[i].name == name) return null;
		}

		StreamCollHeader header;
		header.offset = to->pos();
		header.length = 0;
		header.name = name;
		headers.push_back(header);

		inputStream = new CollStream(to, false, header.offset, 0);
		return inputStream;
	}

	void StreamCollWriter::saveHeader() {
		nat64 headerOffset = to->pos();

		to->write<nat>(headers.size());

		for (nat i = 0; i < headers.size(); i++) {
			to->write<String>(headers[i].name);
			to->write<nat64>(headers[i].offset);
			to->write<nat64>(headers[i].length);
		}

		to->write<nat64>(headerOffset);
	}

	void StreamCollWriter::closeInput() {
		if (inputStream == null) return;

		StreamCollHeader &last = headers.back();
		last.length = to->pos() - last.offset;

		hasAnyError |= inputStream->error();

		del(inputStream);
	}

	bool StreamCollWriter::addStream(Stream *from, const String &name) {
		Stream *tmp = open(name);
		if (tmp == null) return false;

		tmp->write(from);

		return !tmp->error();
	}
}
