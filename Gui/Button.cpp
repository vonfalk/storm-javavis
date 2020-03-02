#include "stdafx.h"
#include "Button.h"
#include "Container.h"
#include "GtkSignal.h"

namespace gui {

	Button::Button(Str *title) : isDefault(false) {
		text(title);
		onClick = null;
	}

	Button::Button(Str *title, Fn<void, Button *> *click) : isDefault(false) {
		text(title);
		onClick = click;
	}

	void Button::clicked() {
		if (onClick)
			onClick->call(this);
	}

#ifdef GUI_WIN32

	bool Button::create(Container *parent, nat id) {
		DWORD flags = buttonFlags;
		if (isDefault)
			flags |= BS_DEFPUSHBUTTON;
		return Window::createEx(WC_BUTTON, flags, 0, parent->handle().hwnd(), id);
	}

	bool Button::onCommand(nat id) {
		if (id == BN_CLICKED) {
			clicked();
			return true;
		}

		return false;
	}

	Size Button::minSize() {
		Size sz = font()->stringSize(text());
		sz.w += sz.h;
		sz.h *= 1.6f;
		return sz;
	}

	void Button::setDefault(Bool def) {
		isDefault = def;
		if (created()) {
			LONG flags = GetWindowLong(handle().hwnd(), GWL_STYLE);
			if (def)
				flags |= BS_DEFPUSHBUTTON;
			else
				flags &= ~(BS_DEFPUSHBUTTON);
			SetWindowLong(handle().hwnd(), GWL_STYLE, flags);
		}
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

	Size Button::minSize() {
		gint w = 0, h = 0;

		if (created()) {
			gtk_widget_get_preferred_width(handle().widget(), &w, NULL);
			gtk_widget_get_preferred_height(handle().widget(), &h, NULL);
		}

		return Size(Float(w), Float(h));
	}

	void Button::setDefault(Bool def) {
		isDefault = def;
		// We don't need anything more here for Gtk+.
	}

#endif

}
