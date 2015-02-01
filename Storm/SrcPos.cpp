#include "stdafx.h"
#include "SrcPos.h"

#include "Utils/TextFile.h"
#include "Utils/FileStream.h"

namespace storm {

	LineCol::LineCol(nat line, nat col) : line(line), col(col) {}

	void LineCol::output(std::wostream &to) const {
		to << "(" << line << ", " << col << ")";
	}

	/**
	 * SrcPos.
	 */

	SrcPos::SrcPos() : sharedFile(null), offset(noOffset) {}

	SrcPos::SrcPos(const Path &file, nat offset) : offset(offset) {
		sharedFile = shared(file);
	}

	SrcPos::SrcPos(const SrcPos &o) : sharedFile(o.sharedFile), offset(o.offset) {
		if (sharedFile) {
			Lock::L z(cacheLock);
			sharedFile->refs++;
		}
	}

	SrcPos &SrcPos::operator =(const SrcPos &o) {
		release(sharedFile);

		Lock::L z(cacheLock);
		offset = o.offset;
		sharedFile = o.sharedFile;
		sharedFile->refs++;

		return *this;
	}

	SrcPos::~SrcPos() {
		release(sharedFile);
	}

	bool SrcPos::mapCompare::operator ()(const File *a, const File *b) const {
		return *a->file < *b->file;
	}

	SrcPos::File *SrcPos::shared(const Path &p) {
		Lock::L z(cacheLock);

		File f = { &p, 0 };
		File *result;
		FileCache::const_iterator i = fileCache.find(&f);
		if (i == fileCache.end()) {
			result = new File(f);
			result->file = new Path(p);
			fileCache.insert(result);
		} else {
			result = *i;
		}
		result->refs++;
		return result;
	}

	void SrcPos::release(File *f) {
		if (f) {
			Lock::L z(cacheLock);
			if (--f->refs == 0) {
				fileCache.erase(f);
				delete f->file;
				delete f;
			}
		}
	}

	Path SrcPos::file() const {
		if (sharedFile)
			return *sharedFile->file;
		else
			return Path();
	}

	SrcPos SrcPos::operator +(nat o) const {
		SrcPos p(*this);
		p.offset += o;
		return p;
	}

	wostream &operator <<(wostream &to, const SrcPos &pos) {
		if (pos.unknown())
			to << "<unknown location>";
		else
			to << pos.file() << "(" << pos.offset << ")";
		return to;
	}

	LineCol SrcPos::lineCol() const {
		LineCol r(0, 0);
		if (unknown())
			return r;

		TextReader *reader = null;

		try {
			reader = TextReader::create(new FileStream(file(), Stream::mRead));
			for (nat i = 0; i < offset; i++) {
				wchar c = reader->get();
				switch (c) {
				case '\r':
					// ignored.
					break;
				case '\n':
					r.line++;
					r.col = 0;
					break;
				default:
					r.col++;
					break;
				}
			}
		} catch (...) {
			if (reader)
				delete reader;
			throw;
		}

		if (reader)
			delete reader;

		return r;
	}

	SrcPos::FileCache SrcPos::fileCache;
	Lock SrcPos::cacheLock;

}
