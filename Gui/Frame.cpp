#include "stdafx.h"
#include "Frame.h"
#include "App.h"
#include "GtkSignal.h"
#include "Exception.h"

namespace gui {

	static void goFull(Handle wnd, Nat &oldStyle, Rect &oldPos);
	static void goBack(Handle wnd, Nat &oldStyle, Rect &oldPos);


	Frame::Frame(Str *title) : myMenu(null), full(false), showCursor(true) {
		onClose = new (this) Event();
		attachParent(this);
		text(title);
		pos(Rect(-10000, -10000, -10002, -10002));
		posSet = false;
	}

	Frame::Frame(Str *title, Size size) : myMenu(null), full(false), showCursor(true) {
		onClose = new (this) Event();
		attachParent(this);
		text(title);
		pos(Rect(Point(-10000, -10000), size));
		posSet = false;
	}

	void Frame::create() {
		if (handle() != invalid)
			return;

		createWindow(true, null);
	}

	void Frame::close() {
		if (handle() != invalid) {
			handle(invalid);
		}

		onClose->set();
	}

	void Frame::waitForClose() {
		App *app = gui::app(engine());
		app->waitForEvent(this, onClose);
	}

	void Frame::size(Size s) {
		if (!full) {
			Bool oldSet = posSet;
			Rect p = pos();
			p.size(s);
			pos(p);
			posSet = oldSet;
		}
	}

	Bool Frame::cursorVisible() {
		return showCursor;
	}

	void Frame::fullscreen(Bool v) {
		if (created() && full != v) {
			if (v) {
				goFull(handle(), windowedStyle, windowedRect);
			} else {
				goBack(handle(), windowedStyle, windowedRect);
			}
		}

		full = v;
	}

	Bool Frame::fullscreen() {
		return full;
	}

	void Frame::menu(MenuBar *menu) {
		if (menu && menu->attachedTo && menu != myMenu)
			throw new (this) GuiError(S("A menu can only be attached to one frame at a time."));

		MenuBar *old = myMenu;
		if (old)
			old->attachedTo = null;

		myMenu = menu;
		if (menu)
			menu->attachedTo = this;

		if (created()) {
			setMenu(old);
		}
	}

	MenuBar *Frame::menu() {
		return myMenu;
	}

	Menu::Item *Frame::findMenuItem(Handle h) {
		if (myMenu) {
			if (Menu::Item *found = myMenu->findMenuItem(h)) {
				return found;
			}
		}

		if (myPopup) {
			if (Menu::Item *found = myPopup->findMenuItem(h)) {
				// This will not cover the case when a menu is closed without any selection, but it
				// seems it is the best we can do.
				myPopup = null;
				return found;
			}
		}

		return null;
	}

	void Frame::onResize(Size size) {
		updateMinSize();
		Window::onResize(size);
	}

	void Frame::destroyWindow(Handle handle) {
		// Remove the menu first, so we don't destroy that as well (only an issue on Windows).
		MenuBar *old = myMenu;
		myMenu = null;
		setMenu(old);
		// Restore, so we don't alter the visible state.
		myMenu = old;

		Window::destroyWindow(handle);
	}


#ifdef GUI_WIN32

	void Frame::pos(Rect r) {
		if (full)
			return;

		posSet = true;

		// We need to do this to take the menu into account.
		myPos = r;
		if (created()) {
			RECT z = convert(r);
			HWND h = handle().hwnd();
			DWORD style = GetWindowLong(h, GWL_STYLE);
			DWORD exStyle = GetWindowLong(h, GWL_EXSTYLE);
			AdjustWindowRectEx(&z, style, myMenu ? TRUE : FALSE, exStyle);
			// Todo: keep track if we need to repaint.
			MoveWindow(h, z.left, z.top, z.right - z.left, z.bottom - z.top, TRUE);
		}
	}

	// Convert a client size to the actual window size.
	static Size clientToWindow(Size s, DWORD windowStyles, DWORD windowExStyles, bool menu) {
		RECT r = { 0, 0, (LONG)s.w, (LONG)s.h };
		BOOL m = menu ? TRUE : FALSE;
		AdjustWindowRectEx(&r, windowStyles & ~WS_OVERLAPPED, menu, windowExStyles);
		return Size(Float(r.right - r.left), Float(r.bottom - r.top));
	}

	// Make 'wnd' go fullscreen.
	static void goFull(Handle wnd, Nat &oldStyle, Rect &oldPos) {
		oldStyle = GetWindowLong(wnd.hwnd(), GWL_STYLE);
		RECT oldRect;
		GetWindowRect(wnd.hwnd(), &oldRect);
		oldPos = convert(oldRect);

		HMONITOR m = MonitorFromWindow(wnd.hwnd(), MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(m, &mi);

		SetWindowLong(wnd.hwnd(), GWL_STYLE, oldStyle & ~(WS_CAPTION | WS_THICKFRAME));

		LONG l = mi.rcMonitor.left;
		LONG r = mi.rcMonitor.right;
		LONG t = mi.rcMonitor.top;
		LONG b = mi.rcMonitor.bottom;
		SetWindowPos(wnd.hwnd(), NULL, l, t, r - l, b - t, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	static void goBack(Handle wnd, Nat &style, Rect &rect) {
		LONG l = LONG(rect.p0.x);
		LONG r = LONG(rect.p1.x);
		LONG t = LONG(rect.p0.y);
		LONG b = LONG(rect.p1.y);

		SetWindowLong(wnd.hwnd(), GWL_STYLE, LONG(style));
		SetWindowPos(wnd.hwnd(), NULL, l, t, r - l, b - t, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	// Helper to create the actual handle.
	bool Frame::createWindow(bool sizeable, MAYBE(Frame *) parent) {
		DWORD exStyles = WS_EX_CONTROLPARENT;
		DWORD styles = 0;
		if (sizeable)
			styles = WS_OVERLAPPEDWINDOW;
		else
			styles = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

		styles |= WS_CLIPCHILDREN;

		// Respect the minimum size.
		updateMinSize();
		Size sz = myPos.size();
		sz.w = max(sz.w, lastMinSize.w);
		sz.h = max(sz.h, lastMinSize.h);
		myPos.size(sz);

		HWND hParent = NULL;
		if (parent)
			hParent = parent->handle().hwnd();
		CreateFlags cFlags = cManualVisibility;
		if (!posSet)
			cFlags |= cAutoPos;
		if (!createEx(NULL, styles, exStyles, hParent, 0, cFlags))
			return false;

		// Create child windows (little of a hack, sorry!)
		parentCreated(0);


		if (visible()) {
			ShowWindow(handle().hwnd(), TRUE);
			update();
		}

		if (full) {
			goFull(handle(), windowedStyle, windowedRect);
		}

		if (myMenu) {
			setMenu(null);
		}

		return true;
	}

	static void fill(MINMAXINFO *info, HWND hWnd, Size size, bool menu) {
		// Need to adjust 'size' since it is measured in client size, while Win32 expects actual window sizes.
		LONG styles = GetWindowLong(hWnd, GWL_STYLE);
		LONG exStyles = GetWindowLong(hWnd, GWL_EXSTYLE);
		size = clientToWindow(size, styles, exStyles, menu);

		info->ptMinTrackSize.x = (LONG)size.w;
		info->ptMinTrackSize.y = (LONG)size.h;
	}

	MsgResult Frame::onMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_CLOSE:
			close();
			return msgResult(0);
		case WM_SETCURSOR:
			if (LOWORD(msg.lParam) == HTCLIENT && !showCursor) {
				SetCursor(NULL);
				return msgResult(TRUE);
			}
			break;
		case WM_GETMINMAXINFO:
			fill((MINMAXINFO *)msg.lParam, handle().hwnd(), lastMinSize, myMenu != null);
			return msgResult(0);
		case WM_MENUCOMMAND:
			menuClicked((HMENU)msg.lParam, (Nat)msg.wParam);
			return msgResult(TRUE);
		}

		return Container::onMessage(msg);
	}

	void Frame::cursorVisible(Bool v) {
		// Will be altered soon anyway, we do not need to do anything here.
		showCursor = v;
	}

	void Frame::updateMinSize() {
		lastMinSize = minSize();
	}

	void Frame::setMenu(MenuBar *) {
		// Check the size of the window now.
		RECT original;
		GetClientRect(handle().hwnd(), &original);

		// Note: SetMenu does not remove the old menu. That is exactly what we want.
		if (myMenu) {
			SetMenu(handle().hwnd(), myMenu->handle.menu());
		} else {
			SetMenu(handle().hwnd(), NULL);
		}

		// Check the new size.
		RECT changed;
		GetClientRect(handle().hwnd(), &changed);

		// Alter our height if needed.
		int delta = original.bottom - changed.bottom;
		if (delta && !full) {
			GetWindowRect(handle().hwnd(), &original);
			original.bottom += delta;
			int width = original.right - original.left;
			int height = original.bottom - original.top;
			SetWindowPos(handle().hwnd(), HWND_TOP, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
		}
	}

	void Frame::menuClicked(HMENU menu, Nat id) {
		if (!myMenu)
			return;

		if (Menu *m = myMenu->findMenu(menu)) {
			if (id >= m->count())
				return;

			(*m)[id]->clicked();
		}
	}

	void Frame::popupMenu(PopupMenu *menu) {
		if (!created())
			throw new (this) GuiError(S("Can not show a popup menu from a window that is not created."));

		POINT pt;
		GetCursorPos(&pt);
		RECT limits;
		GetWindowRect(handle().hwnd(), &limits);

		if (pt.x < limits.left)
			pt.x = limits.left;
		if (pt.x > limits.right - 1)
			pt.x = limits.right - 1;
		if (pt.y < limits.top)
			pt.y = limits.top;
		if (pt.y > limits.bottom - 1)
			pt.y = limits.bottom - 1;

		TrackPopupMenuEx(menu->handle.menu(), TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, handle().hwnd(), NULL);
	}


#endif
#ifdef GUI_GTK

	void Frame::pos(Rect r) {
		myPos = r;
		posSet = true;
		if (!full && handle() != Handle()) {
			gtk_window_resize(GTK_WINDOW(handle().widget()), r.size().w, r.size().h);
		}
	}

	static void goFull(Handle wnd, Nat &, Rect &) {
		GdkWindow *window = gtk_widget_get_window(wnd.widget());
		gdk_window_set_fullscreen_mode(window, GDK_FULLSCREEN_ON_CURRENT_MONITOR);
		gdk_window_fullscreen(window);
	}

	static void goBack(Handle wnd, Nat &, Rect &) {
		GdkWindow *window = gtk_widget_get_window(wnd.widget());
		gdk_window_unfullscreen(window);
	}

	bool Frame::createWindow(bool sizeable, MAYBE(Frame *) parent) {
		GtkWidget *frame = null;
		// If "parent" is set, we want to create a dialog instead.
		if (parent) {
			// We use "with buttons" so that we may specify flags and parent!
			const char *title = text()->utf8_str();
			GtkDialogFlags flags = GTK_DIALOG_MODAL;
			GtkWindow *parentWindow = GTK_WINDOW(parent->handle().widget());
			// The last NULL is not needed, but GCC complains if we omit it.
			frame = gtk_dialog_new_with_buttons(title, parentWindow, flags, NULL, NULL);

			// A dialog already contains a Bin container. So we remove that...
			GtkWidget *contained = gtk_bin_get_child(GTK_BIN(frame));
			gtk_container_remove(GTK_CONTAINER(frame), contained);
		} else {
			frame = gtk_window_new(GTK_WINDOW_TOPLEVEL);
			gtk_window_set_title((GtkWindow *)frame, text()->utf8_str());
		}
		gtk_window_set_resizable((GtkWindow *)frame, sizeable);

		// Add a VBox to the frame, so we can place the menu appropriately.
		GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_container_add(GTK_CONTAINER(frame), vbox);
		gtk_widget_show(vbox);

		// Place a BASIC container inside, so that we can control layout of child widgets as we want.
		GtkWidget *container = basic_new();
		gtk_box_pack_end(GTK_BOX(vbox), container, true, true, 0);
		gtk_widget_show(container);

		Signal<void, Frame>::Connect<&Frame::close>::to(frame, "destroy", engine());

		Size sz = pos().size();
		if (sz.w > 0 && sz.h > 0)
			gtk_window_set_default_size((GtkWindow *)frame, sz.w, sz.h);

		handle(frame);
		initSignals(frame, container);

		// Create child windows (if any).
		parentCreated(0);

		updateMinSize();

		// Set visibility and full screen attributes.
		if (visible()) {
			gtk_widget_show(frame);
		}

		// Update cursor visibility.
		cursorVisible(showCursor);

		if (full) {
			goFull(handle(), windowedStyle, windowedRect);
		}

		if (myMenu) {
			setMenu(null);
		}

		return true;
	}

	void Frame::text(Str *str) {
		if (created()) {
			gtk_window_set_title((GtkWindow *)handle().widget(), str->utf8_str());
		}
		Window::text(str);
	}

	static void getLastCB(GtkWidget *widget, gpointer data) {
		GtkWidget **store = (GtkWidget **)data;
		*store = widget;
	}

	static Basic *getContainer(GtkWidget *root) {
		GtkWidget *found = null;
		GtkWidget *vbox = gtk_bin_get_child(GTK_BIN(root));
		gtk_container_foreach(GTK_CONTAINER(vbox), &getLastCB, &found);
		// There is a BASIC layout at the last position.
		return BASIC(found);
	}

	void Frame::addChild(GtkWidget *child, Rect pos) {
		Basic *basic = getContainer(handle().widget());
		Size s = pos.size();
		basic_put(basic, child, pos.p0.x, pos.p0.y, s.w, s.h);
	}

	void Frame::moveChild(GtkWidget *child, Rect pos) {
		Basic *basic = getContainer(handle().widget());
		Size s = pos.size();
		basic_move(basic, child, pos.p0.x, pos.p0.y, s.w, s.h);
	}

	GtkWidget *Frame::drawWidget() {
		GtkWidget *found = null;
		GtkWidget *vbox = gtk_bin_get_child(GTK_BIN(handle().widget()));
		gtk_container_foreach(GTK_CONTAINER(vbox), &getLastCB, &found);
		return found;
	}

	void Frame::cursorVisible(Bool v) {
		showCursor = v;
		if (created()) {
			GtkWidget *widget = handle().widget();
			GdkCursor *cursor = null;
			if (showCursor)
				cursor = gdk_cursor_new_from_name(gtk_widget_get_display(widget), "default");
			else
				cursor = gdk_cursor_new_for_display(gtk_widget_get_display(widget), GDK_BLANK_CURSOR);

			GdkWindow *window = gtk_widget_get_window(widget);

			gdk_window_set_cursor(window, cursor);
		}
	}

	void Frame::updateMinSize() {
		lastMinSize = minSize();

		if (handle() != Handle()) {
			GtkWidget *widget = drawWidget();
			gtk_widget_set_size_request(widget, (gint)lastMinSize.w, (gint)lastMinSize.h);
		}
	}

	void Frame::setMenu(MenuBar *old) {
		GtkWidget *vbox = gtk_bin_get_child(GTK_BIN(handle().widget()));
		gint heightDiff = 0;
		gint oldWidth, oldHeight;
		gtk_window_get_size(GTK_WINDOW(handle().widget()), &oldWidth, &oldHeight);

		if (old) {
			gint natural;
			gtk_widget_get_preferred_height(old->handle.widget(), NULL, &natural);
			gtk_container_remove(GTK_CONTAINER(vbox), old->handle.widget());

			heightDiff -= natural;
		}

		if (myMenu) {
			// Place the menu first.
			GtkWidget *m = myMenu->handle.widget();
			gtk_box_pack_start(GTK_BOX(vbox), m, false, false, 0);

			gtk_widget_show_all(m);

			gint natural;
			gtk_widget_get_preferred_height(m, NULL, &natural);

			heightDiff += natural;
		}

		if (!full)
			gtk_window_resize(GTK_WINDOW(handle().widget()), oldWidth, oldHeight + heightDiff);

	}

	void Frame::popupMenu(PopupMenu *menu) {
		if (!created())
			throw new (this) GuiError(S("Can not show a popup menu from a window that is not created."));

		// Note: We will keep the popup alive a bit too long here, as it is difficult to get a
		// definitive answer as to when the popup menu is closed.
		myPopup = menu;
		gtk_menu_popup_at_pointer(GTK_MENU(menu->handle.widget()), NULL);
	}

#endif

}
