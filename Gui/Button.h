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

	protected:
#ifdef GUI_WIN32
		virtual bool create(HWND parent, nat id);
#endif
	};

}
