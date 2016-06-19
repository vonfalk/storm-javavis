#pragma once
#include "Utils/Path.h"

/**
 * Position in a source file.
 */
class SrcPos {
public:
	// Unknown position.
	explicit SrcPos();

	// Give a path-id and an offset.
	SrcPos(nat fileId, nat offset);

	// No file and no offset.
	static const nat invalid = -1;

	// File #.
	nat fileId;

	// Offset.
	nat offset;

	// All files.
	static vector<Path> files;

	inline bool operator ==(const SrcPos &o) const { return fileId == o.fileId && offset == o.offset; }
	inline bool operator !=(const SrcPos &o) const { return !(*this == o); }
};

wostream &operator <<(wostream &to, const SrcPos &pos);
