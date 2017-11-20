#include "stdafx.h"
#include "Button.h"
#include "GtkSignal.h"

namespace gui {

	Button::Button(Str *title) {
		text(title);
		onClick = null;
	}

	Button::Button(Str *title, Fn<void, Button *> *click) {
		text(title);
		onClick = click;
	}

	void Button::clicked() {
		if (onClick)
			onClick->call(this);
	}

#ifdef GUI_WIN32
	bool Button::create(Container *parent, nat id) {
		return Window::createEx(WC_BUTTON, buttonFlags, 0, parent->handle().hwnd(), id);
	}

	bool Button::onCommand(nat id) {
		if (id == BN_CLICKED) {
			clicked();
			return true;
		}

		return false;
	}
#endif

#ifdef GUI_GTK

	bool Button::create(Container *parent, nat id) {
		GtkWidget *button = gtk_button_new_with_label(text()->utf8_str());
		initWidget(parent, button);
		Signal<void, Button>::Connect<&Button::clicked>::to(button, "clicked", engine());
		return true;
	}

	void Button::text(Str *text) {
		if (created()) {
			GtkWidget *widget = gtk_bin_get_child(GTK_BIN(handle().widget()));
			gtk_label_set_text(GTK_LABEL(widget), text->utf8_str());
		}
		Window::text(text);
	}

#endif

}
