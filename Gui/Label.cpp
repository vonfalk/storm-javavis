#include "stdafx.h"
#include "Label.h"
#include "Container.h"
#include "Win32Dpi.h"

namespace gui {

	Label::Label(Str *title) {
		text(title);
	}

#ifdef GUI_WIN32

	bool Label::create(ContainerBase *parent, nat id) {
		return createEx(WC_STATIC, childFlags, 0, parent->handle().hwnd(), id);
	}

	Size Label::minSize() {
		Str *s = text();
		Bool hasNewline = false;
		for (Str::Iter i = s->begin(), e = s->end(); i != e; ++i) {
			if (i.v() == Char('\n')) {
				hasNewline = true;
				break;
			}
		}

		if (hasNewline) {
			Nat dpi = currentDpi();
			// Text layout does not always scale linearly.
			return dpiFromPx(dpi, font()->stringSize(text(), dpi));
		} else {
			// If no newlines, the difference is small enough to ignore. It causes issues in the layout.
			return font()->stringSize(text());
		}
	}

#endif
#ifdef GUI_GTK

	bool Label::create(ContainerBase *parent, nat id) {
		GtkWidget *label = gtk_label_new(text()->utf8_str());
		gtk_widget_set_halign(label, (GtkAlign)GTK_ALIGN_START);

		GValue zero = G_VALUE_INIT;
		g_value_init(&zero, G_TYPE_FLOAT);
		g_value_set_float(&zero, (gfloat)0.0f);
		g_object_set_property(G_OBJECT(label), "xalign", &zero);
		g_object_set_property(G_OBJECT(label), "yalign", &zero);
		g_value_unset(&zero);

		initWidget(parent, label);
		return true;
	}

	void Label::text(Str *text) {
		if (created()) {
			gtk_label_set_text(GTK_LABEL(handle().widget()), text->utf8_str());
		}
		Window::text(text);
	}

	Size Label::minSize() {
		gint w = 0, h = 0;

		if (created()) {
			gtk_widget_get_preferred_width(handle().widget(), &w, NULL);
			gtk_widget_get_preferred_height(handle().widget(), &h, NULL);
		}

		return Size(Float(w), Float(h));
	}

#endif
}
