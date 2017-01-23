#pragma once
#include "Utils/Bitmask.h"
#include "Shared/Object.h"
#include "Shared/Str.h"
#include "Shared/Array.h"
#include "Utils/Exception.h"
#include "Protocol.h"
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	class Protocol;

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
		// Flags describing the surroundings of 'parts'.
		enum Flags {
			nothing = 0x00,

			// Is this a directory (ie trailing backslash?). It is, however, always possible to
			// append another part to a non-directory for convenience reasons.
			isDir = 0x01,
		};

		// Empty url.
		STORM_CTOR Url();

		// Create from fundamentals. (TODO: Storm needs the flags somehow!)
		Url(Par<Protocol> p, Par<ArrayP<Str>> parts, Flags flags);

		// Ctor for STORM.
		STORM_CTOR Url(Par<Protocol> p, Par<ArrayP<Str>> parts);

		// Copy ctor.
		STORM_CTOR Url(Par<Url> o);

		// Dtor.
		~Url();

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> o);

		// Equals. Note that this is always a bitwise equality, multiple paths may name
		// the same file!
		virtual Bool STORM_FN equals(Par<Object> o);

		// Append another path to this one. The other one has to be a relative path.
		Url *STORM_FN push(Par<Url> url);
		Url *STORM_FN operator /(Par<Url> url);

		// Append another part to this one.
		Url *STORM_FN push(Par<Str> part);
		Url *push(const String &part);
		Url *STORM_FN operator /(Par<Str> url);

		// Append another part to this one, making the resulting Url into a directory.
		Url *STORM_FN pushDir(Par<Str> part);
		Url *pushDir(const String &part);

		// Get all parts.
		ArrayP<Str> *STORM_FN getParts() const;

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
		Url *relative(Par<Url> to);

		/**
		 * Find out things about this URL. All operations are not always supported
		 * by all protocols. Note that these are generally assumed to be run on non-relative urls.
		 */

		// Find all children URL:s.
		virtual ArrayP<Url> *STORM_FN children();

		// Open this Url for reading.
		virtual IStream *STORM_FN read();

		// Open this Url for writing.
		virtual OStream *STORM_FN write();

		// Does this Url exist?
		virtual Bool STORM_FN exists();

		// Format for other C-api:s. May not work for all kinds of URL:s.
		virtual String format();

	protected:
		// Output.
		virtual void output(wostream &to) const;

	private:
		// Protocol. If null, we're a relative path!
		Auto<Protocol> protocol;

		// Parts of the path.
		Auto<ArrayP<Str>> parts;

		// Flags
		Flags flags;

		// Make a copy we can modify.
		Url *copy();
	};

	BITMASK_OPERATORS(Url::Flags);

	// Parse a path. (assumed to be on the local file system).
	// TODO: We need something general!
	Url *parsePath(Engine &e, const String &s);
	Url *STORM_FN parsePath(Par<Str> s);

	// Get some good URL:s.
	Url *executableFileUrl(Engine &e);
	Url *executableUrl(Engine &e);
	Url *dbgRootUrl(Engine &e);

		// Some exceptions.
	class InvalidName : public Exception {
	public:
		InvalidName(const String &name) : name(name) {}
		String name;
		String what() const { return L"The url part " + name + L" is not acceptable."; }
	};

}