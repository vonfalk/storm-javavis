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

		virtual GtkWidget *fontWidget();
#endif

		// Get minimum size.
		virtual Size STORM_FN minSize();

	protected:
		virtual bool create(ContainerBase *parent, nat id);

	private:
		// Click notification.
		void clicked();

		// Set this button as the default choice in a dialog. Called by "Dialog".
		void setDefault(Bool def);

		// Is this the default button in the dialog?
		Bool isDefault;

		friend class Dialog;
	};

}
