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

	SrcPos::SrcPos(const Path &file, nat offset) : file(file), offset(offset) {}

	Path SrcPos::file() const {
		return ;
	}

	void SrcPos::output(std::wostream &to) const {
		to << file() << "(" << offset << ")";
	}

	LineCol SrcPos::lineCol() const {
		LineCol r(0, 0);
		util::TextReader *reader = null;

		try {
			reader = util::TextReader::create(new util::FileStream(file, util::Stream::mRead));
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

		return r;
	}

}
