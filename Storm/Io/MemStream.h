#pragma once
#include "Stream.h"

namespace storm {

	/**
	 * Simple memory stream.
	 */
	class IMemStream : public IStream {
		STORM_CLASS;
	public:
		// Create from a buffer.
		STORM_CTOR IMemStream(const Buffer &b);

		// More?
		virtual Bool STORM_FN more();

		// Read
		virtual Nat STORM_FN read(Buffer &to);

		// Peek.
		virtual Nat STORM_FN peek(Buffer &to);

	private:
		// Data
		Buffer data;

		// Position.
		nat pos;
	};

}
