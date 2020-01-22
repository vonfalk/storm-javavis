#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Core/EnginePtr.h"
#include "Utils/Exception.h"
#include "Utils/Bitmask.h"

namespace storm {
	STORM_PKG(core.io);

	class Protocol;
	class IStream;
	class OStream;

	// Flags describing the surroundings of 'parts'.
	enum UrlFlags {
		nothing = 0x00,

		// Is this a directory (ie trailing backslash?). It is, however, always possible to
		// append another part to a non-directory for convenience reasons.
		isDir = 0x01,
	};

	BITMASK_OPERATORS(UrlFlags);

	/**
	 * An URL represents a URL in a system-independent format that is easy to manipulate
	 * programmatically. The 'protocol' part in the URL specifies what backend that should
	 * handle requests to open the specified URL. The object is designed to be treated
	 * as an immutable object.
	 * To keep it simple, this object will always output the unix path separator (/). Correctly
	 * formatting URL:s for use with file systems is left to the implementation of protocols.
	 */
	class Url : public Object {
		STORM_CLASS;
	public:

		// Empty url.
		STORM_CTOR Url();

		// Create from fundamentals.
		STORM_CTOR Url(Protocol* p, Array<Str *> *parts, UrlFlags flags);

		// Relative path creation.
		STORM_CTOR Url(Array<Str *> *parts, UrlFlags flags);

		// Ctor for STORM.
		STORM_CTOR Url(Protocol *p, Array<Str *> *parts);

		// Relative path.
		STORM_CTOR Url(Array<Str *> *parts);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *o);

		// Equals. Note that this is always a bitwise equality, multiple paths may name
		// the same file!
		virtual Bool STORM_FN operator ==(const Url &o) const;

		// Hash of this url.
		virtual Nat STORM_FN hash() const;

		// Append another path to this one. The other one has to be a relative path.
		Url *STORM_FN push(Url *url);
		Url *STORM_FN operator /(Url *url);

		// Append another part to this one.
		Url *STORM_FN push(Str *part);
		Url *STORM_FN operator /(Str *url);

		// Append another part to this one, making the resulting Url into a directory.
		Url *STORM_FN pushDir(Str *part);

		// Get all parts.
		Array<Str *> *STORM_FN getParts() const;

		// Is this a directory?
		Bool STORM_FN dir() const;

		// Absolute path?
		Bool STORM_FN absolute() const;

		// Parent directory.
		Url *STORM_FN parent() const;

		// Get the file title. The title does not contain the file extension.
		Str *STORM_FN title() const;

		// Get the file name, this includes the extension.
		Str *STORM_FN name() const;

		// Get the file extension.
		Str *STORM_FN ext() const;

		// Generate a relative path.
		Url *relative(Url *to);

		/**
		 * Low-level operations.
		 */

		// Get the number of parts in this URL.
		inline Nat STORM_FN count() const { return parts->count(); }
		inline Bool STORM_FN empty() const { return parts->empty(); }
		inline Bool STORM_FN any() const { return parts->any(); }

		// Get a specific part.
		inline Str *at(Nat i) const { return parts->at(i); }
		inline Str *STORM_FN operator[](Nat i) const { return parts->at(i); }

		/**
		 * Find out things about this URL. All operations are not always supported
		 * by all protocols. Note that these are generally assumed to be run on non-relative urls.
		 */

		// Find all children URL:s.
		virtual Array<Url *> *STORM_FN children();

		// Open this Url for reading.
		virtual IStream *STORM_FN read();

		// Open this Url for writing.
		virtual OStream *STORM_FN write();

		// Does this Url exist?
		virtual Bool STORM_FN exists();

		// Format for other C-api:s. May not work for all kind of URL:s.
		virtual Str *STORM_FN format();

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Protocol. If null, we're a relative path!
		Protocol *protocol;

		// Parts of the path.
		Array<Str *> *parts;

		// Flags
		UrlFlags flags;

		// Make a copy we can modify.
		Url *copy();
	};

	// Parse a path. (assumed to be on the local file system).
	// TODO: We need something general!
	Url *STORM_FN parsePath(Str *s);
	Url *parsePath(Engine &e, const wchar *str);

	// Get some good URL:s.
	Url *executableFileUrl(Engine &e);
	Url *executableUrl(Engine &e);
	Url *dbgRootUrl(Engine &e);
	Url *STORM_FN cwdUrl(EnginePtr e);

	// Some exceptions.
	class EXCEPTION_EXPORT InvalidName : public NException {
		STORM_CLASS;
	public:
		STORM_CTOR InvalidName() { name = null; }
		STORM_CTOR InvalidName(Str *name) { this->name = name; }

		virtual void STORM_FN message(StrBuf *to) const {
			if (!name)
				*to << S("Empty name segments are not allowed.");
			else
				*to << S("The url part ") << name << S(" is not acceptable.");
		}
	private:
		MAYBE(Str *) name;
	};

}
