#pragma once

#include "Stream.h"

namespace storm {

	/**
	 * File IO. This is tightly linked to the FileProtocol, and the classes should be created from there.
	 */
	class IFileStream : public RIStream {
		STORM_CLASS;
	public:
		// Create. Create from an Url.
		IFileStream(const String &name);

		// Copy...
		STORM_CTOR IFileStream(Par<IFileStream> o);

		// Destroy.
		~IFileStream();

		// More?
		virtual Bool STORM_FN more();

		// Read.
		virtual Nat STORM_FN read(Buffer &to);

		// Peek.
		virtual Nat STORM_FN peek(Buffer &to);

		// Seek.
		virtual void STORM_FN seek(Word to);

		// Tell.
		virtual Word STORM_FN tell();

		// Length.
		virtual Word STORM_FN length();

	protected:
		// Output.
		virtual void output(wostream &to) const;

	private:
		// File name.
		String name;

		// System independent handle.
		void *handle;
	};

	class OFileStream : public OStream {
		STORM_CLASS;
	public:
		// Create.
		OFileStream(const String &name);

		// Copy...
		STORM_CTOR OFileStream(Par<OFileStream> o);

		// Destroy.
		~OFileStream();

		virtual void STORM_FN write(const Buffer &buf);

	protected:
		// Output.
		virtual void output(wostream &to) const;

	private:
		// File name.
		String name;

		// System independent handle.
		void *handle;
	};

}
