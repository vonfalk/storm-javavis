#pragma once

#include "Storm/Lib/Object.h"
#include "Storm/Lib/Array.h"
#include "Utils/Exception.h"
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	class Url;


	/**
	 * Error thrown when an operation is not supported by a protocol.
	 */
	class ProtocolNotSupported : public Exception {
	public:
		ProtocolNotSupported(const String &operation, const String &protocol) : operation(operation), protocol(protocol) {}
		String operation, protocol;
		virtual String what() const { return operation + L" is not supported by the protocol " + protocol; }
	};

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

		/**
		 * These should be implemented as far as possible.
		 */

		// Children of this directory.
		virtual ArrayP<Url> *STORM_FN children(Par<Url> url);

		// Read a file.
		virtual IStream *STORM_FN read(Par<Url> url);

		// Write a file.
		virtual OStream *STORM_FN write(Par<Url> url);

		// Exists?
		virtual Bool STORM_FN exists(Par<Url> url);
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

		// Children of this directory.
		virtual ArrayP<Url> *STORM_FN children(Par<Url> url);

		// Read a file.
		virtual IStream *STORM_FN read(Par<Url> url);

		// Write a file.
		virtual OStream *STORM_FN write(Par<Url> url);

		// Exists?
		virtual Bool STORM_FN exists(Par<Url> url);

		// Convert an Url to a string suitable for other C-api:s.
		String format(Par<Url> url);

	protected:
		// output
		virtual void output(wostream &to) const;
	};


}
