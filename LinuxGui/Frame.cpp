#include "stdafx.h"
#include "Frame.h"

namespace gui {

	Frame::Frame(Str *title) {
		text(title);
	}

	void Frame::create() {
		if (created())
			return;

		GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

		// TODO: See if we should be visible...
		gtk_widget_show(window);

		handle(window);
	}

	void Frame::close() {}

}
