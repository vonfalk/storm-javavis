#pragma once
#include "Window.h"

namespace stormgui {

	/**
	 * Label for displaying text.
	 */
	class Label : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR Label(Par<Str> text);

	protected:
		virtual bool create(HWND parent, nat id);
	};

}
