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

		// Notifications.
		virtual bool onCommand(nat id);

	protected:
		virtual bool create(HWND parent, nat id);
	};

}
