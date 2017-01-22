#pragma once
#include "Window.h"

namespace stormgui {

	/**
	 * Button.
	 */
	class Button : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR Button(Par<Str> title);
		STORM_CTOR Button(Par<Str> title, Par<FnPtr<void, Par<Button>>> click);

		// Click callback.
		STORM_VAR Auto<FnPtr<void, Par<Button>>> onClick;

		// Notifications.
		virtual bool onCommand(nat id);

	protected:
		virtual bool create(HWND parent, nat id);
	};

}
