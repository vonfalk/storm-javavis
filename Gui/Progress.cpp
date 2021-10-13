#include "stdafx.h"
#include "Progress.h"
#include "Container.h"
#include "Win32Dpi.h"

namespace gui {

	Progress::Progress() : myValue(0), myMax(100), myUnknown(false) {}

	void Progress::v(Nat v) {
		value(v);
	}

#ifdef GUI_WIN32

	static const DWORD marqueeTime = 16;

	bool Progress::create(ContainerBase *parent, nat id) {
		DWORD flags = childFlags;

		if (myUnknown)
			flags |= PBS_MARQUEE;

		if (!createEx(PROGRESS_CLASS, flags, 0, parent->handle().hwnd(), id))
			return false;

		if (myUnknown) {
			SendMessage(handle().hwnd(), PBM_SETMARQUEE, 1, marqueeTime);
		} else {
			SendMessage(handle().hwnd(), PBM_SETRANGE32, 0, myMax);
			SendMessage(handle().hwnd(), PBM_SETPOS, myValue, 0);
		}

		return true;
	}

	Size Progress::minSize() {
		return dpiFromPx(currentDpi(), Size(200, 20));
	}

	void Progress::value(Nat v) {
		myValue = std::min(v, Nat(0x7FFFFFFF));

		if (created())
			SendMessage(handle().hwnd(), PBM_SETPOS, myValue, 0);
	}

	void Progress::max(Nat v) {
		myMax = std::min(v, Nat(0x7FFFFFFF));

		if (created())
			SendMessage(handle().hwnd(), PBM_SETRANGE32, 0, myMax);
	}

	void Progress::unknown(Bool v) {
		myUnknown = v;

		if (created()) {
			if (myUnknown) {
				SetWindowLong(handle().hwnd(), GWL_STYLE, childFlags | PBS_MARQUEE);
				SendMessage(handle().hwnd(), PBM_SETMARQUEE, 1, marqueeTime);
			} else {
				SendMessage(handle().hwnd(), PBM_SETMARQUEE, 0, 0);
				SetWindowLong(handle().hwnd(), GWL_STYLE, childFlags);
				SendMessage(handle().hwnd(), PBM_SETRANGE32, 0, myMax);
				SendMessage(handle().hwnd(), PBM_SETPOS, myValue, 0);
			}
		}
	}

#endif
#ifdef GUI_GTK

	bool Progress::create(ContainerBase *parent, nat id) {
		GtkWidget *widget = gtk_progress_bar_new();
		initWidget(parent, widget);
		return true;
	}

	Size Progress::minSize() {
		gint w = 0, h = 0;

		if (created()) {
			gtk_widget_get_preferred_width(handle().widget(), &w, NULL);
			gtk_widget_get_preferred_height(handle().widget(), &h, NULL);
		}

		return Size(Float(w), Float(h));
	}

#endif

}
