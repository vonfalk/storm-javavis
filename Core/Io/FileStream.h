#pragma once
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * File IO.
	 */
	class IFileStream : public RIStream {
		STORM_CLASS;
	public:
		// Create from a path.
		IFileStream(Str *name);

		// Copy...
		IFileStream(const IFileStream &o);

		// Destroy.
		virtual ~IFileStream();

		// More?
		virtual Bool STORM_FN more();

		// Read.
		using RIStream::read;
		virtual Buffer STORM_FN read(Buffer b, Nat start);

		// Peek.
		using RIStream::peek;
		virtual Buffer STORM_FN peek(Buffer b, Nat start);

		// Seek.
		virtual void STORM_FN seek(Word to);

		// Tell.
		virtual Word STORM_FN tell();

		// Length.
		virtual Word STORM_FN length();

		// Random access.
		virtual RIStream *STORM_FN randomAccess();

		// Close.
		virtual void STORM_FN close();

	private:
		// File name.
		Str *name;

		// System dependent handle.
		void *handle;
	};

}
