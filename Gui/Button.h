#pragma once
#include "Window.h"
#include "Core/Fn.h"

namespace gui {

	/**
	 * Button.
	 */
	class Button : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR Button(Str *title);
		STORM_CTOR Button(Str *title, Fn<void, Button *> *click);

		// Click callback.
		MAYBE(Fn<void, Button *> *) onClick;

#ifdef GUI_WIN32
		// Notifications.
		virtual bool onCommand(nat id);
#endif
#ifdef GUI_GTK
		using Window::text;
		void text(Str *text);
#endif

	protected:
		virtual bool create(Container *parent, nat id);

	private:
		// Click notification.
		void clicked();
	};

}
