#pragma once
#include "Core/Exception.h"

namespace sql {

	class EXCEPTION_EXPORT SQLError : public storm::Exception {
		STORM_EXCEPTION;
	public:
		STORM_CTOR SQLError(Str *msg);

		virtual void STORM_FN message(StrBuf *to) const;

	private:
		Str *msg;
	};

}
