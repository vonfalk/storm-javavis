#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Utils/Exception.h"

namespace storm {
	STORM_PKG(core.io);

	class Url;
	class IStream;
	class OStream;

	/**
	 * Error thrown when an operation is not supported by a protocol.
	 */
	class EXCEPTION_EXPORT ProtocolNotSupported : public Exception {
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

		// Compare two parts of a filename for equality.
		// Implemented here as a simple bitwise comparision.
		virtual Bool STORM_FN partEq(Str *a, Str *b);

		// Hash a part of a filename.
		virtual Nat STORM_FN partHash(Str *a);

		/**
		 * These should be implemented as far as possible.
		 */

		// Children of this directory.
		virtual Array<Url *> *STORM_FN children(Url *url);

		// Read a file.
		virtual IStream *STORM_FN read(Url *url);

		// Write a file.
		virtual OStream *STORM_FN write(Url *url);

		// Exists?
		virtual Bool STORM_FN exists(Url *url);

		// Convert to a string suitable for other C-api:s
		virtual Str *STORM_FN format(Url *url);

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// The same protocol as some other?
		virtual Bool STORM_FN operator ==(const Protocol &o) const;
	};


	/**
	 * The 'file' protocol. On Windows, this causes case-insensitive file name compares.
	 */
	class FileProtocol : public Protocol {
		STORM_CLASS;
	public:
		// Ctor.
		STORM_CTOR FileProtocol();

		// Compare parts.
		virtual Bool STORM_FN partEq(Str *a, Str *b);

		// Hash parts.
		virtual Nat STORM_FN partHash(Str *a);

		// Children of this directory.
		virtual Array<Url *> *STORM_FN children(Url *url);

		// Read a file.
		virtual IStream *STORM_FN read(Url *url);

		// Write a file.
		virtual OStream *STORM_FN write(Url *url);

		// Exists?
		virtual Bool STORM_FN exists(Url *url);

		// Convert an Url to a string suitable for other C-api:s.
		virtual Str *STORM_FN format(Url *url);

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

}
