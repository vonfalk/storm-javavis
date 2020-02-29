#include "stdafx.h"
#include "Dialog.h"
#include "Exception.h"

namespace gui {

	Dialog::Dialog(Str *title) : Frame(title), parent(null) {}

	Dialog::Dialog(Str *title, Size size) : Frame(title, size), parent(null) {}

	void Dialog::close() {
		close(-1);
	}

	void Dialog::defaultChoice(Button *button) {
		if (defaultButton)
			defaultButton->setDefault(false);
		button->setDefault(true);
		button->onClick = fnPtr(engine(), &Dialog::onOk, this);

		setDefault(button);
		defaultButton = button;
	}

	void Dialog::onOk(Button *) {
		close(1);
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

	MsgResult Dialog::beforeMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_KEYDOWN:
		{
			Nat key = keycode(msg.wParam);
			Modifiers mod = modifiers();
			if (mod == mod::none && key == key::esc) {
				close();
				return msgResult(TRUE);
			} else if (mod == mod::none && key == key::ret) {
				if (defaultButton) {
					// Simulate a click.
					defaultButton->onCommand(BN_CLICKED);
					return msgResult(TRUE);
				}
			}
		}
		}

		return Frame::onMessage(msg);
	}

	void Dialog::setDefault(Button *) {
		// No need on Win32.
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

	void Dialog::setDefault(Button *def) {
		gtk_dialog_set_default_response(GTK_DIALOG(handle().widget()), 1);
	}

#endif

}
