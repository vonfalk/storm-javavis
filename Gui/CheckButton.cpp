#include "stdafx.h"
#include "CheckButton.h"
#include "Container.h"

namespace gui {

	CheckButton::CheckButton(Str *title) {
		text(title);
	}

	CheckButton::CheckButton(Str *title, Fn<void, Bool> *change) {
		text(title);
		onChange = change;
	}

	void CheckButton::changed(Bool to) {
		if (onChange)
			onChange->call(to);
	}

	void CheckButton::toggled() {
		changed(checked());
	}

#ifdef GUI_WIN32

	bool CheckButton::create(ContainerBase *parent, nat id) {
		DWORD flags = controlFlags | BS_AUTOCHECKBOX;
		if (Window::createEx(WC_BUTTON, flags, 0, parent->handle().hwnd(), id)) {
			SendMessage(handle().hwnd(), BM_SETCHECK, isChecked ? BST_CHECKED : BST_UNCHECKED, 0);
			return true;
		}
		return false;
	}

	bool CheckButton::onCommand(nat id) {
		if (id == BN_CLICKED) {
			toggled();
			return true;
		}

		return false;
	}

	Size CheckButton::minSize() {
		Size sz = font()->stringSize(text());
		sz.w += GetSystemMetrics(SM_CXMENUCHECK) + GetSystemMetrics(SM_CXEDGE);
		sz.h = max(sz.h, float(GetSystemMetrics(SM_CYMENUCHECK)));
		return sz;
	}

	Bool CheckButton::checked() {
		if (created()) {
			LRESULT state = SendMessage(handle().hwnd(), BM_GETCHECK, 0, 0);
			isChecked = state == BST_CHECKED;
		}

		return isChecked;
	}

	void CheckButton::checked(Bool v) {
		if (v != isChecked) {
			isChecked = v;
			toggled();
		}

		if (created()) {
			SendMessage(handle().hwnd(), BM_SETCHECK, isChecked ? BST_CHECKED : BST_UNCHECKED, 0);
		}
	}


#endif
#ifdef GUI_GTK

	bool CheckButton::create(ContainerBase *parent, nat id) {
		GtkWidget *button = gtk_check_button_new_with_label(NULL, text()->utf8_str());
		initWidget(parent, button);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), isChecked);
		Signal<void, CheckButton>::Connect<&CheckButton::toggle>::to(button, "toggled", engine());
		return true;
	}

	void CheckButton::text(Str *text) {
		if (created()) {
			GtkWidget *widget = gtk_bin_get_child(GTK_BIN(handle().widget()));
			gtk_label_set_text(GTK_LABEL(widget), text->utf8_str());
		}
		Window::text(text);
	}

	GtkWidget *CheckButton::fontWidget() {
		return gtk_bin_get_child(GTK_BIN(handle().widget()));
	}

	Size CheckButton::minSize() {
		gint w = 0, h = 0;

		if (created()) {
			gtk_widget_get_preferred_width(handle().widget(), &w, NULL);
			gtk_widget_get_preferred_height(handle().widget(), &h, NULL);
		}

		return Size(Float(w), Float(h));
	}

	Bool Button::checked() {
		if (created()) {
			isChecked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(handle().widget())) != 0;
		}
		return isChecked;
	}

	void Button::checked(Bool v) {
		isChecked = v;
		if (created()) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(handle().widget()), isChecked);
		}
	}

#endif

}
