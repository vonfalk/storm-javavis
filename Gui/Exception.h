#pragma once
#include "Core/Exception.h"

namespace gui {

	class EXCEPTION_EXPORT GuiError : public storm::Exception {
		STORM_CLASS;
	public:
		GuiError(const wchar *what) {
			data = new (this) Str(what);
			saveTrace();
		}
		STORM_CTOR GuiError(Str *what) {
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
