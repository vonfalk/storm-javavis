#pragma once

#include "Core/Io/Stream.h"

namespace ssl {

	/**
	 * A stream of cryptographically secure random numbers retrieved from the system.
	 *
	 * Uses CryptGenRandom on Windows, and /dev/urandom on Linux.
	 *
	 * Note: This stream is not buffered in any way, so it is a good idea to read from it in large
	 * chunks.
	 */
	class RandomStream : public IStream {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR RandomStream();

		// Copy.
		RandomStream(const RandomStream &o);

		// Destroy.
		~RandomStream();

		// More data?
		Bool STORM_FN more() override;

		// Read data.
		Buffer STORM_FN read(Buffer to) override;

		// Close.
		void STORM_FN close() override;

	private:
		// Platform-dependent data.
		size_t data;

		// Initialize 'data'.
		void init();
	};

}
