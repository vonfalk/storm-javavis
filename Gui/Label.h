#pragma once
#include "Window.h"

namespace gui {

	/**
	 * Label for displaying text.
	 *
	 * TODO: Allow setting text alignment.
	 */
	class Label : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR Label(Str *text);

#ifdef GUI_GTK
		using Window::text;
		virtual void STORM_FN text(Str *text);
#endif

	protected:
		virtual bool create(Container *parent, nat id);
	};

}
