#pragma once
#include "HandleStream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * File IO.
	 */

	class FileIStream : public HandleRIStream {
		STORM_CLASS;
	public:
		// Create from a path.
		FileIStream(Str *name);

		// Copy.
		FileIStream(const FileIStream &o);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// File name.
		Str *name;
	};


	class FileOStream : public HandleOStream {
		STORM_CLASS;
	public:
		// Create from a path.
		FileOStream(Str *name);

		// Copy.
		FileOStream(const FileOStream &o);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// File name.
		Str *name;
	};

}
