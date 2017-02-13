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
	SrcPos(nat fileId, nat line, nat col);

	// No file and no offset.
	static const nat invalid = -1;

	// File #.
	nat fileId;

	// Line + column.
	nat line, col;

	// All files.
	static vector<Path> files;

	// First file that should be exported and not just used.
	static nat firstExport;

	inline bool operator ==(const SrcPos &o) const { return fileId == o.fileId && line == o.line && col == o.col; }
	inline bool operator !=(const SrcPos &o) const { return !(*this == o); }
};

wostream &operator <<(wostream &to, const SrcPos &pos);
