#pragma once
#include "Utils/Exception.h"

namespace gui {

	class EXCEPTION_EXPORT GuiError : public Exception {
	public:
		GuiError(const String &what) : data(what) {}

		inline virtual String what() const { return data; }
	private:
		String data;
	};

}
