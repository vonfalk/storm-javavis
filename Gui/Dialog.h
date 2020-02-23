#pragma once
#include "Frame.h"

namespace gui {

	/**
	 * A dialog is a Frame that is modal, i.e. it disallows interaction with some parent frame
	 * window until it is closed. The dialog also implements "show()", which blocks until the dialog
	 * is closed, and returns some value to the caller, which represents the result of the dialog.
	 */
	class Dialog : public Frame {
		STORM_CLASS;
	public:
		// Create. The dialog is not shown until "show" is called.
		STORM_CTOR Dialog(Str *title);
		STORM_CTOR Dialog(Str *title, Size size);

		// Show the dialog as a modal window. Don't call "create" before calling "show", that is handled internally.
		Int STORM_FN show(Frame *parent);

		// Call when the window is shown to close the dialog with a particular code.
		void STORM_FN close(Int result);

		// Override the default behavior to return -1.
		virtual void STORM_FN close();

	private:
		// Our parent. Only valid while we're being shown.
		MAYBE(Frame *) parent;

		// Remember the result.
		Int result;
	};

}
