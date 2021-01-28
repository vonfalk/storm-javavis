#pragma once

#ifdef POSIX

namespace ssl {

	// Get a string for a certificate error code.
	const wchar *certError(long code);

}

#endif
