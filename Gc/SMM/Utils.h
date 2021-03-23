#pragma once

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

#if defined(WINDOWS)
#define LAST_ERROR_T DWORD
#define LAST_ERROR_READ() GetLastError()
#define LAST_ERROR_WRITE(x) SetLastError(x)
#elif defined(POSIX)
#define LAST_ERROR_T int
#define LAST_ERROR_READ() errno
#define LAST_ERROR_WRITE(x) errno = x
#endif

		/**
		 * Automatically save and restore "errno" or "GetLastError" as appropriate for the current
		 * platform.
		 */
		class SaveLastError {
		public:
			SaveLastError() {
				lastError = LAST_ERROR_READ();
			}

			~SaveLastError() {
				LAST_ERROR_WRITE(lastError);
			}

		private:
			LAST_ERROR_T lastError;
		};

	}
}

#endif
