#pragma once
#include "Window.h"
#include "Align.h"

namespace gui {

	/**
	 * Label for displaying text.
	 *
	 * Default alignment is top-left.
	 */
	class Label : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR Label(Str *text);
		STORM_CTOR Label(Str *text, HAlign halign);
		STORM_CTOR Label(Str *text, VAlign valign);
		STORM_CTOR Label(Str *text, HAlign halign, VAlign valign);

#ifdef GUI_GTK
		using Window::text;
		virtual void STORM_FN text(Str *text);
#endif

		virtual Size STORM_FN minSize();

	protected:
		virtual bool create(ContainerBase *parent, nat id);

	private:
		HAlign hAlign;
		VAlign vAlign;
	};

}
