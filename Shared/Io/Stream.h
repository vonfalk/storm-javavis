#pragma once
#include "Shared/Object.h"

namespace storm {
	STORM_PKG(storm.io);

	/**
	 * Input and Output streams for Storm. These are abstract base classes
	 * and concrete implementations are provided for at least file system IO
	 * and standard input/output.
	 *
	 * When cloning streams, you get a new read-pointer to the underlying source.
	 * TODO: In some cases (such as networks) this does not makes sense...
	 */

	/**
	 * Basic buffer type to allow fast and efficient interoperability between
	 * storm and C++. This type is a fixed-size array and its size in a value-type
	 * so that we may pass it to and from Storm efficiently.
	 * The buffer object has strict ownership of its contents, and therefore has
	 * to be copied around all the time. Be careful with copies of this one!
	 */
	class Buffer {
		STORM_VALUE;
	public:
		// Create with a pre-allocated buffer (eg on the stack). Only from C++.
		// You are still in charge of deallocating the buffer!
		Buffer(void *buffer, nat size);

		// Create with a specific size.
		STORM_CTOR Buffer(Nat size);

		// Copy.
		Buffer(const Buffer &o);

		// Assign.
		Buffer &operator =(const Buffer &o);

		// Destroy.
		~Buffer();

		// Number of bytes.
		inline Nat STORM_FN count() const { return size; }

		// Element access.
		Byte &STORM_FN operator [](Nat id) { return data[id]; }
		const Byte &operator [](Nat id) const { return data[id]; }

		// Pointer to the first element.
		byte *dataPtr() const { return data; }

	private:
		// Contents.
		byte *data;

		// Size.
		nat size;

		// Do we own this buffer?
		bool owner;
	};

	// Output
	wostream &operator <<(wostream &to, const Buffer &b);


	/**
	 * Input stream.
	 */
	class IStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR IStream();

		// Copy.
		STORM_CTOR IStream(Par<IStream> o);

		// Are we at the end of the stream?
		virtual Bool STORM_FN more();

		// Read a buffer from the stream. Returns the number of bytes read.
		virtual Nat STORM_FN read(Buffer &to);

		// Peek data.
		virtual Nat STORM_FN peek(Buffer &to);
	};


	/**
	 * Output stream.
	 */
	class OStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR OStream();

		// Copy.
		STORM_CTOR OStream(Par<OStream> o);

		// Write some data.
		virtual void STORM_FN write(const Buffer &buf);
	};

}
