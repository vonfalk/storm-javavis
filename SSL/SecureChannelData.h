#pragma once
#include "Core/Io/Stream.h"
#include "Core/Io/Buffer.h"

namespace ssl {

	/**
	 * GC Data needed by the Secure Channel implementation.
	 *
	 * This is located in a separate header file as we always need to include it in the binary due
	 * to the preprocessor.
	 */
	class SChannelData : public Object {
		STORM_CLASS;
	public:
		// Create.
		SChannelData(IStream *src, OStream *dst)
			: src(src), dst(dst),
			  decryptedStart(0), decryptedEnd(0), remainingStart(0),
			  incomingEnd(false), outgoingEnd(false) {}


		// Input stream.
		IStream *src;

		// Output stream.
		OStream *dst;

		// Buffer for outgoing data. Contents here are not persistent, we keep it around to avoid
		// memory allocations. It may be empty.
		Buffer outgoing;

		// This buffer is where we store data we have received from the network, and are in the
		// process of decrypting. The data consists of a number of parts, namely:
		// - decrypted plaintext - from decryptedStart to decryptedEnd.
		// - ciphertext - from remainingStart to filled()
		Buffer incoming;

		// Start and end of decrypted data in 'incoming'.
		Nat decryptedStart;
		Nat decryptedEnd;

		// Start of ciphertext in 'incoming'.
		Nat remainingStart;

		// Guideline on buffer size.
		Nat bufferSizes;

		// Is the read end shut down?
		Bool incomingEnd;

		// Is the write end shut down?
		Bool outgoingEnd;
	};

}
