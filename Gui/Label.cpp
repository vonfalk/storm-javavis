#include "stdafx.h"
#include "Label.h"

namespace gui {

	Label::Label(Str *title) {
		text(title);
	}

#ifdef GUI_WIN32
	bool Label::create(Container *parent, nat id) {
		return createEx(WC_STATIC, childFlags, 0, parent->handle().hwnd(), id);
	}
#endif
#ifdef GUI_GTK
	bool Label::create(Container *parent, nat id) {
		TODO(L"The default should be top-left aligned. Not vertically centered.");
		GtkWidget *label = gtk_label_new(text()->utf8_str());
		gtk_widget_set_halign(label, (GtkAlign)GTK_ALIGN_START);
		initWidget(parent, label);
		return true;
	}

	void Label::text(Str *text) {
		if (created()) {
			gtk_label_set_text(GTK_LABEL(handle().widget()), text->utf8_str());
		}
		Window::text(text);
	}
#endif
}
