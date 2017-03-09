#pragma once
#include "Window.h"

namespace gui {

	/**
	 * Label for displaying text.
	 */
	class Label : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR Label(Str *text);

	protected:
		virtual bool create(HWND parent, nat id);
	};

}
