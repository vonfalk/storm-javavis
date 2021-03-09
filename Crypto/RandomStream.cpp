#include "stdafx.h"
#include "RandomStream.h"
#include "Core/Exception.h"

#if defined(WINDOWS)

#define SECURITY_WIN32
#include <Wincrypt.h>

#elif defined(POSIX)

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#endif

namespace ssl {

	RandomStream::RandomStream() : data(0) {
		init();
	}

	RandomStream::RandomStream(const RandomStream &) : data(0) {
		// There is no point in "copying" the old stream...
		init();
	}

	RandomStream::~RandomStream() {
		close();
	}

	Bool RandomStream::more() {
		return data != 0;
	}

#if defined(WINDOWS)

	void RandomStream::init() {
		HCRYPTPROV provider = NULL;
		if (!CryptAcquireContext(&provider, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
			throw new (this) InternalError(S("Failed to acquire a cryptographic context for random numbers."));

		data = size_t(provider);
	}

	Buffer RandomStream::read(Buffer to) {
		if (data == 0)
			return to;

		Nat fill = to.free();
		if (!CryptGenRandom(HCRYPTPROV(data), fill, to.dataPtr() + to.filled()))
			throw new (this) InternalError(S("Failed to generate random data."));

		to.filled(to.filled() + fill);

		return to;
	}

	void RandomStream::close() {
		if (data)
			CryptReleaseContext(HCRYPTPROV(data), 0);
		data = 0;
	}

#elif defined(POSIX)

	void RandomStream::init() {
		int fd = ::open("/dev/urandom", O_RDONLY);
		if (fd < 0)
			throw new (this) InternalError(S("Failed to open /dev/urandom to acquire randomness."));

		fcntl(fd, F_SETFD, FD_CLOEXEC);
		data = size_t(fd + 1);
	}

	Buffer RandomStream::read(Buffer to) {
		if (data == 0)
			return to;

		Nat fill = to.free();
		Bool done = false;

		while (!done) {
			// Note: This is fine since "/dev/urandom" typically does not block.
			ssize_t result = ::read(int(data - 1), to.dataPtr() + to.filled(), fill);
			if (result < 0) {
				if (errno == EINTR)
					continue;
				throw new (this) InternalError(S("Failed to read from /dev/urandom!"));
			} else {
				done = true;
				to.filled(to.filled() + result);
			}
		}

		return to;
	}

	void RandomStream::close() {
		if (data > 0)
			::close(int(data - 1));
		data = 0;
	}

#endif

}
