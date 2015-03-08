#include "stdafx.h"
#include "Future.h"

#ifdef CUSTOM_EXCEPTION_PTR
// Found in crt/src/tidtable.c
extern "C" void * __cdecl _getptd();
// Look in crt/src/mtdll.h for the _tiddata definition!
#endif

namespace code {

	Future::Future(void *target, nat size)
		: targetPtr(target), size(size), resultSema(0),
#ifdef CUSTOM_EXCEPTION_PTR
		  errorSema(0),
#endif
		  hasError(false) {}

	void Future::wait() {
		resultSema.down();
		if (hasError) {
			throwError();
		}
	}

	void Future::result() {
		hasError = false;
		resultSema.up();
	}

#ifdef CUSTOM_EXCEPTION_PTR

	int Future::filter(EXCEPTION_POINTERS *ptrs) {
		EXCEPTION_RECORD *record = ptrs->ExceptionRecord;

		// Only handle C++ exceptions.
		if (record->ExceptionCode != 0xE06D7363)
			return EXCEPTION_CONTINUE_SEARCH;

		if (record->ExceptionInformation[1] == 0) {
			// Rethrown exception...
			// _tiddata *ptd = _gettid();
			void *ptd = _getptd();
			// record = ptd->_curexception;
			record = OFFSET_IN(ptd, 0x88, EXCEPTION_RECORD *);
			OFFSET_IN(ptd, 0x88, EXCEPTION_RECORD *) = null;
		}

		errorData = *record;

		// Prevent double-destroying the data.
		ptrs->ExceptionRecord->ExceptionCode = 0;
		record->ExceptionCode = 0;

		return EXCEPTION_EXECUTE_HANDLER;
	}

	void Future::error() {
		hasError = true;

		__try {
			throw;
		} __except (filter(GetExceptionInformation())) {
			// Notify result.
			resultSema.up();
			// Wait until someone has consumed the error.
			errorSema.down();
			// Sleep(1000);
		}
	}


	void Future::throwError() {
		try {
			RaiseException(errorData.ExceptionCode,
						errorData.ExceptionFlags,
						errorData.NumberParameters,
						errorData.ExceptionInformation);
		} catch (...) {
			errorSema.up();
			throw;
		}
	}

#else

	void Future::throwError() {
		std::rethrow_exception(errorData);
	}

	void Future::error() {
		hasError = true;
		errorData = std::current_exception();
		resultSema.up();
	}

#endif

}
