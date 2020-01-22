#pragma once
#include "Core/Exception.h"

namespace gui {

	class EXCEPTION_EXPORT GuiError : public NException {
		STORM_CLASS;
	public:
		GuiError(const wchar *what) {
			data = new (this) Str(what);
		}
		STORM_CTOR GuiError(Str *what) {
			data = what;
		}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << data;
		}
	private:
		Str *data;
	};

}
