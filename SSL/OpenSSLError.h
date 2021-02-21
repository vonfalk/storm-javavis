#pragma once

#ifdef POSIX

namespace ssl {

	// Get a string for a certificate error code.
	const wchar *certError(long code);

	// Throw a suitable exception on SSL error if one exists.
	void checkError();

	// Throw a suitable exception always.
	void throwError();
}

#endif
