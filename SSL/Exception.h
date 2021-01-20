#pragma once
#include "Core/Exception.h"

namespace ssl {

	class EXCEPTION_EXPORT SSLError : public storm::Exception {
		STORM_EXCEPTION;
	public:
		SSLError(const wchar *what) {
			data = new (this) Str(what);
			saveTrace();
		}

		STORM_CTOR SSLError(Str *what) {
			data = what;
			saveTrace();
		}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << data;
		}

	private:
		Str *data;
	};

}
