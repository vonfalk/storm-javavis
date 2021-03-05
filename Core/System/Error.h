#pragma once

#include "Core/Exception.h"

namespace storm {
	STORM_PKG(core.sys);

	/**
	 * Exception used for system errors.
	 */
	class EXCEPTION_EXPORT SystemError : public Exception {
		STORM_EXCEPTION;
	public:
		SystemError(const wchar *msg);
		STORM_CTOR SystemError(Str *msg);

		virtual void STORM_FN message(StrBuf *to) const;
	private:
		Str *msg;
	};

}
