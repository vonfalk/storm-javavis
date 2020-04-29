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
	SrcPos(nat fileId, nat pos);

	// No file and no offset.
	static const nat invalid = -1;

	// File #.
	nat fileId;

	// Character offsets (newlines count as 1 character regardless of their representation)
	nat pos;

	// All files.
	static vector<Path> files;

	// First file that should be exported and not just used.
	static nat firstExport;

	inline bool operator ==(const SrcPos &o) const { return fileId == o.fileId && pos == o.pos; }
	inline bool operator !=(const SrcPos &o) const { return !(*this == o); }
};

wostream &operator <<(wostream &to, const SrcPos &pos);
