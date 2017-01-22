#pragma once

namespace stormgui {

	class GuiError : public Exception {
	public:
		GuiError(const String &what) : data(what) {}

		inline virtual String what() const { return data; }
	private:
		String data;
	};

}
