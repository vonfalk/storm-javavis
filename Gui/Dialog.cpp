#include "stdafx.h"
#include "Dialog.h"
#include "Exception.h"

namespace gui {

	Dialog::Dialog(Str *title) : Frame(title), parent(null) {}

	Dialog::Dialog(Str *title, Size size) : Frame(title, size), parent(null) {}

	void Dialog::close() {
		close(-1);
	}

#ifdef GUI_WIN32

	Int Dialog::show(Frame *parent) {
		if (handle() != invalid)
			throw new (this) GuiError(S("Don't call 'create' before calling 'show' on a dialog."));

		this->parent = parent;
		createWindow(true, parent);

		// Make us modal!
		EnableWindow(parent->handle().hwnd(), FALSE);
		waitForClose();
		this->parent = null;
		return result;
	}

	void Dialog::close(Int result) {
		this->result = result;
		// Make sure to enable the old window before we remove ourselves.
		if (parent)
			EnableWindow(parent->handle().hwnd(), TRUE);
		Frame::close();
	}

#endif

#ifdef GUI_GTK

	Int Dialog::show(Frame *parent) {
		if (handle() != invalid)
			throw new (this) GuiError(S("Don't call 'create' before calling 'show' on a dialog."));

		this->parent = parent;
		createWindow(true, parent);

		gint result = gtk_dialog_run(GTK_DIALOG(handle().widget()));

		// This is the default "close" action.
		if (result == -4)
			result = -1;

		// Close the window.
		Frame::close();

		this->parent = null;
		return result;
	}

	void Dialog::close(Int result) {
		gtk_dialog_response(GTK_DIALOG(handle().widget()), result);
	}

#endif

}
