#pragma once
#include "HandleStream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * File IO.
	 */

	class IFileStream : public HandleRIStream {
		STORM_CLASS;
	public:
		// Create from a path.
		IFileStream(Str *name);

		// Copy.
		IFileStream(const IFileStream &o);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// File name.
		Str *name;
	};


	class OFileStream : public HandleOStream {
		STORM_CLASS;
	public:
		// Create from a path.
		OFileStream(Str *name);

		// Copy.
		OFileStream(const OFileStream &o);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// File name.
		Str *name;
	};

}
