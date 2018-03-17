#include "stdafx.h"
#include "Button.h"
#include "Container.h"
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

	Size Button::minSize() const {
		SIZE sz;
		Button_GetIdealSize(handle().hwnd(), &sz);

		return Size(Float(sz.cx), Float(sz.cy));
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

	GtkWidget *Button::fontWidget() {
		return gtk_bin_get_child(GTK_BIN(handle().widget()));
	}

	Size Button::minSize() const {
		gint w, h;

		gtk_widget_get_preferred_width(handle().widget(), &w, NULL);
		gtk_widget_get_preferred_height(handle().widget(), &h, NULL);

		return Size(Float(w), Float(h));
	}

#endif

}
