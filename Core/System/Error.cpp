#include "stdafx.h"
#include "Error.h"

namespace storm {

	SystemError::SystemError(const wchar *msg) {
		this->msg = new (this) Str(msg);
		saveTrace();
	}

	SystemError::SystemError(Str *msg) {
		this->msg = msg;
		saveTrace();
	}

	void SystemError::message(StrBuf *to) const {
		*to << S("System error: ") << msg;
	}

}
