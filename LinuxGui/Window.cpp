#include "stdafx.h"
#include "Window.h"

namespace gui {

	Window::Window() {
		myText = new (this) Str(L"");
	}

	Window::~Window() {
		if (created()) {
			// TODO: Remove this window from the App object.
			gtk_widget_destroy(myHandle.widget());
		}
	}

	GtkWidget *Window::handle() const {
		return myHandle.widget();
	}

	void Window::handle(GtkWidget *widget) {
		myHandle = Handle(widget);
	}

	const Str *Window::text() {
		return myText;
	}

	void Window::text(Str *s) {
		myText = s;
	}

}
