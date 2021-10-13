#include "stdafx.h"
#include "Progress.h"
#include "Container.h"
#include "Win32Dpi.h"

namespace gui {

	Progress::Progress() : myProgress(0.0f), waitMode(false), timerId(0) {}

#ifdef GUI_WIN32

	static const DWORD marqueeTime = 16;
	static const WORD progressMax = 0xFFFF;

	bool Progress::create(ContainerBase *parent, nat id) {
		DWORD flags = childFlags;

		if (waitMode)
			flags |= PBS_MARQUEE;

		if (!createEx(PROGRESS_CLASS, flags, 0, parent->handle().hwnd(), id))
			return false;

		if (waitMode) {
			SendMessage(handle().hwnd(), PBM_SETMARQUEE, 1, marqueeTime);
		} else {
			SendMessage(handle().hwnd(), PBM_SETRANGE32, 0, progressMax);
			SendMessage(handle().hwnd(), PBM_SETPOS, Nat(progressMax * myProgress), 0);
		}

		return true;
	}

	void Progress::windowDestroyed() {}

	Size Progress::minSize() {
		return dpiFromPx(currentDpi(), Size(200, 20));
	}

	void Progress::progress(Float v) {
		myProgress = v;

		if (created()) {
			if (waitMode) {
				SendMessage(handle().hwnd(), PBM_SETMARQUEE, 0, 0);
				SetWindowLong(handle().hwnd(), GWL_STYLE, childFlags);
				SendMessage(handle().hwnd(), PBM_SETRANGE32, 0, progressMax);
			}
			SendMessage(handle().hwnd(), PBM_SETPOS, Nat(progressMax * myProgress), 0);
		}

		waitMode = false;
	}

	void Progress::wait() {

		if (created()) {
			if (!waitMode) {
				SetWindowLong(handle().hwnd(), GWL_STYLE, childFlags | PBS_MARQUEE);
				SendMessage(handle().hwnd(), PBM_SETMARQUEE, 1, marqueeTime);
			}
		}

		waitMode = true;
	}

#endif
#ifdef GUI_GTK

	bool Progress::create(ContainerBase *parent, nat id) {
		GtkWidget *widget = gtk_progress_bar_new();
		initWidget(parent, widget);

		if (waitMode) {
			wait();
		} else {
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(widget), myProgress);
		}

		return true;
	}

	void Progress::windowDestroyed() {
		if (timerId) {
			if (GSource *s = g_main_context_find_source_by_id(NULL, timerId))
				g_source_destroy(s);
			timerId = 0;
		}
	}

	Size Progress::minSize() {
		gint w = 0, h = 0;

		if (created()) {
			gtk_widget_get_preferred_width(handle().widget(), &w, NULL);
			gtk_widget_get_preferred_height(handle().widget(), &h, NULL);
		}

		return Size(Float(w), Float(h));
	}

	void Progress::progress(Float v) {
		myProgress = v;
		waitMode = false;

		if (created()) {
			if (timerId) {
				if (GSource *s = g_main_context_find_source_by_id(NULL, timerId))
					g_source_destroy(s);
				timerId = 0;
			}

			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(handle().widget()), myProgress);
		}
	}

	static gboolean progressTimeout(gpointer widget) {
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(widget));
		return G_SOURCE_CONTINUE;
	}

	void Progress::wait() {
		waitMode = true;

		if (created()) {
			gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(handle().widget()), 0.02);
			gtk_progress_bar_pulse(GTK_PROGRESS_BAR(handle().widget()));

			if (timerId == 0) {
				timerId = g_timeout_add(16, &progressTimeout, handle().widget());
			}
		}
	}

#endif

}
