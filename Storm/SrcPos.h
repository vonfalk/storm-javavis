#pragma once

#include "Utils/Path.h"

namespace storm {

	/**
	 * Struct to hold the line and column in a file.
	 */
	class LineCol : public Printable {
	public:
		// Create.
		LineCol(nat line, nat col);

		// The line and column in a file.
		nat line, col;

		// Comparision
		inline bool operator !=(const LineCol &o) const { return !(*this == o); }
		inline bool operator ==(const LineCol &o) const { return line == o.line && col == o.col; }
	protected:
		virtual void output(std::wostream &to) const;
	};

	/**
	 * Describes a position in the source file.
	 * TODO: This representation may need to be optimized a bit, since the current implementation
	 * results in loads of copies of the path to the file.
	 */
	class SrcPos : public Printable {
	public:
		// Given the offset
		SrcPos(const Path &file, nat offset);

		// The file.
		Path file;

		// The offset (in characters) from the beginning of the file. Not counting any BOM.
		nat offset;

		// Compute the line and character offsets by opening the file and count the lines.
		LineCol lineCol() const;

		// Compare.
		inline bool operator !=(const SrcPos &o) const { return !(*this == o); }
		inline bool operator ==(const SrcPos &o) const { return file == o.file && offset == o.offset; }

	protected:
		virtual void output(std::wostream &to) const;
	};

}
