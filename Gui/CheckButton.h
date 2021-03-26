#pragma once
#include "Window.h"
#include "Core/Fn.h"

namespace gui {

	/**
	 * Check button.
	 */
	class CheckButton : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR CheckButton(Str *title);
		STORM_CTOR CheckButton(Str *title, Fn<void, Bool> *onChange);

		// Changed callback. Called whenever this check button changes state.
		MAYBE(Fn<void, Bool> *) onChange;

#ifdef GUI_WIN32
		virtual bool onCommand(nat id);
#endif
#ifdef GUI_GTK
		using Window::text;
		void text(Str *text);

		virtual GtkWidget *fontWidget();
#endif

		// Checked?
		Bool STORM_FN checked();

		// Set checked.
		void STORM_ASSIGN checked(Bool check);

		// Minimum size.
		virtual Size STORM_FN minSize();

	protected:
		virtual bool create(ContainerBase *parent, nat id);

	private:
		// Change notification.
		void changed(Bool state);

		// Called when the button is toggled. Checks the state and calls 'changed'.
		void toggled();

		// Checked?
		Bool isChecked;
	};

}
