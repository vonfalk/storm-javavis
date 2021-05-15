#include "stdafx.h"
#include "Utils/Platform.h"

#if defined(STORM_COMPAT) && defined(LINUX)

#include <fcntl.h>
#include <stdarg.h>

/**
 * On 64-bit machines, F_SETLK == F_SETLK64 since off_t is 64-bit in all cases. Thus, we don't need
 * to worry about not covering all cases. We check this here in case we accidentally compile this
 * file on a 32-bit system.
 */
#if (F_GETLK != F_GETLK64) || (F_SETLK != F_SETLK64)
#error "The wrapper assumes a 64-bit system!"
#endif


/**
 * This wrapper is called from sqlite. Thus, we don't need to worry too much about completeness. The
 * functionality below is what sqlite uses (based on inspecting the code), and assumes that the
 * caller intends to call the 64-bit version of the syscall.
 */
extern "C" int sqlite_fcntl_wrap(int fd, int cmd, ...) {
	int res;
	va_list l;
	va_start(l, cmd);

	// These are the only ones sqlite use.
	switch (cmd) {
	case F_SETFD:
	case F_GETFD:
#ifdef F_FULLFSYNC
		// This is only on OSX, I think. It is in sqlite, so we might as well have it here as well.
	case F_FULLFSYNC:
#endif
		res = fcntl(fd, cmd, va_arg(l, int));
		break;
	case F_GETLK:
		res = fcntl(fd, F_GETLK64, va_arg(l, struct flock *));
		break;
	case F_SETLK:
		res = fcntl(fd, F_SETLK64, va_arg(l, struct flock *));
		break;
	case F_SETLKW:
		res = fcntl(fd, F_SETLKW64, va_arg(l, struct flock *));
		break;
	default:
		dbg_assert(false, "Unknown parameter to fcntl. This wrapper is only intended for sqlite!");
		res = -1;
		break;
	}

	va_end(l);
	return res;
}

#endif
