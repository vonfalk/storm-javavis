#pragma once
#include "Lib/Object.h"

#include "Utils/Path.h"
#include "Utils/Lock.h"

namespace storm {
	STORM_PKG(core.lang);

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
	 */
	class SrcPos {
		STORM_VALUE;
	public:
		// Unknown position.
		explicit SrcPos();

		// Given the offset
		SrcPos(const Path &file, nat offset);

		// Copy
		SrcPos(const SrcPos &o);
		SrcPos &operator =(const SrcPos &o);

		// Dtor
		~SrcPos();

		// Unknown offset.
		static const nat noOffset = -1;

		// Advance a number of characters.
		SrcPos operator +(nat c) const;

		// File.
		Path file() const;

		// The offset (in characters) from the beginning of the file. Not counting any BOM.
		nat offset;

		// Compute the line and character offsets by opening the file and count the lines.
		LineCol lineCol() const;

		// Unknown position?
		inline bool unknown() const { return offset == noOffset; }

		// Compare.
		inline bool operator !=(const SrcPos &o) const { return !(*this == o); }
		inline bool operator ==(const SrcPos &o) const { return offset == o.offset && sharedFile == o.sharedFile; }

	private:
		struct File {
			const Path *file;
			nat refs;
		};

		// Which file are we using?
		File *sharedFile;

		// Compare keys.
		struct mapCompare {
			bool operator() (const File *a, const File *b) const;
		};

		// All files in here. Shared between instances.
		typedef set<File *, mapCompare> FileCache;
		static FileCache fileCache;

		// Lock for the cache.
		static Lock cacheLock;

		// Find the shared Path object.
		File *shared(const Path &path);

		// Release a shared file ref.
		void release(File *f);
	};

	// Output.
	wostream &operator <<(wostream &to, const SrcPos &pos);

}
