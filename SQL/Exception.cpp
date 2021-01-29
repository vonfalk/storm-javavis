#include "stdafx.h"
#include "Exception.h"

namespace sql {

	SQLError::SQLError(Str *msg) : msg(msg) {}

	void SQLError::message(StrBuf *to) const {
		*to << S("SQL error: ") << msg;
	}

}
