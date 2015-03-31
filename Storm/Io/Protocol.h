#pragma once

#include "Storm/Lib/Object.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Defines a protocol to use with a Url. The protocol itself defines how
	 * to access the underlying data. For example, the 'file' protocol opens
	 * regular files.
	 *
	 * The protocol below is an interface. A concrete implementation is provided
	 * for the file system.
	 *
	 * TODO: How to extend?
	 */
	class Protocol : public Object {
		STORM_CLASS;
	public:
		// Ctor.
		STORM_CTOR Protocol();

		// Copy.
		STORM_CTOR Protocol(Par<Protocol> o);

		// Compare two parts of a filename for equality.
		// Implemented here as a simple bitwise comparision.
		virtual Bool STORM_FN partEq(Par<Str> a, Par<Str> b);

	};


	/**
	 * The 'file' protocol. On Windows, this causes case-insensitive file name compares.
	 */
	class FileProtocol : public Protocol {
		STORM_CLASS;
	public:
		// Ctor.
		STORM_CTOR FileProtocol();

		// Copy.
		STORM_CTOR FileProtocol(Par<FileProtocol> p);

		// Compare parts.
		virtual Bool STORM_FN partEq(Par<Str> a, Par<Str> b);

	protected:
		// output
		virtual void output(wostream &to) const;
	};


}
