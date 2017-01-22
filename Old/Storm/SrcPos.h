#pragma once
#include "Lib/Object.h"
#include "Utils/Lock.h"

namespace storm {
	STORM_PKG(core.lang);

	class Url;

	/**
	 * Struct to hold the line and column in a file.
	 */
	class LineCol : public STORM_IGNORE(Printable) {
		STORM_VALUE;
	public:
		// Create.
		STORM_CTOR LineCol(Nat line, Nat col);

		// The line and column in a file.
		Nat line, col;

		// Comparision
		inline Bool operator !=(const LineCol &o) const { return !(*this == o); }
		inline Bool operator ==(const LineCol &o) const { return line == o.line && col == o.col; }
	protected:
		virtual void output(std::wostream &to) const;
	};

	/**
	 * Describes a position in the source file.
	 */
	class SrcPos {
		STORM_VALUE;
	public:
		// Unknown position.
		explicit STORM_CTOR SrcPos();

		// Given the offset
		STORM_CTOR SrcPos(Par<Url> file, Nat offset);

		// Copy
		SrcPos(const SrcPos &o);
		SrcPos &operator =(const SrcPos &o);

		// Dtor, so we do not have to declare Url.
		~SrcPos();

		// Deep copy.
		void STORM_FN deepCopy(Par<CloneEnv> env);

		// Unknown offset.
		static const nat noOffset = -1;

		// Advance a number of characters.
		SrcPos STORM_FN operator +(Nat c) const;

		// File.
		Auto<Url> file;

		// The offset (in characters) from the beginning of the file. Not counting any BOM.
		nat offset;

		// Compute the line and character offsets by opening the file and count the lines.
		LineCol STORM_FN lineCol() const;

		// Unknown position?
		inline Bool STORM_FN unknown() const { return offset == noOffset; }

		// Compare.
		inline Bool STORM_FN operator !=(const SrcPos &o) const { return !(*this == o); }
		Bool STORM_FN operator ==(const SrcPos &o) const;
	};

	// Output.
	wostream &operator <<(wostream &to, const SrcPos &pos);

}
