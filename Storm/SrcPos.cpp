#include "stdafx.h"
#include "SrcPos.h"

#include "Shared/Io/Url.h"
#include "Shared/Io/Text.h"
#include "Shared/CloneEnv.h"
#include "OS/Sync.h"
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

	SrcPos::SrcPos() : file(), offset(noOffset) {}

	SrcPos::SrcPos(Par<Url> file, nat offset) : file(file), offset(offset) {}

	SrcPos::SrcPos(const SrcPos &o) : file(o.file), offset(o.offset) {}

	SrcPos::~SrcPos() {}

	SrcPos &SrcPos::operator =(const SrcPos &o) {
		offset = o.offset;
		file = o.file;

		return *this;
	}

	void SrcPos::deepCopy(Par<CloneEnv> env) {
		clone(file, env);
	}

	SrcPos SrcPos::operator +(nat o) const {
		SrcPos p(*this);
		p.offset += o;
		return p;
	}

	Bool SrcPos::operator ==(const SrcPos &o) const {
		if (offset != o.offset)
			return false;

		if (file && o.file) {
			if (!file->equals(o.file))
				return false;
		} else if (file || o.file) {
			return false;
		}

		return true;
	}

	wostream &operator <<(wostream &to, const SrcPos &pos) {
		if (pos.unknown())
			to << "<unknown location>";
		else
			to << pos.file << "(" << pos.offset << ")";
		return to;
	}

	LineCol SrcPos::lineCol() const {
		LineCol r(0, 0);
		if (unknown())
			return r;

		Auto<IStream> stream = file->read();
		Auto<TextReader> reader = readText(stream);

		for (nat i = 0; i < offset; i++) {
			Nat c = reader->read();
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

		return r;
	}

}
