#include "stdafx.h"
#include "Window.h"
#include "Container.h"
#include "Frame.h"
#include "App.h"
#include "Painter.h"
#include "GtkSignal.h"

// We're using gtk_widget_override_font, as there is no better way of setting custom fonts on widgets...
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace gui {

	// Null is a failure code from eg. GetParent function, should be OK to use as invalid.
	const Handle Window::invalid;

	Window::Window() :
		myHandle(invalid), myParent(null), myRoot(null),
		myVisible(true), gdkWindow(null), myPos(0, 0, 10, 10) {

		myText = new (this) Str(L"");
		myFont = app(engine())->defaultFont;
	}

	Window::~Window() {
		if (myHandle != invalid) {
			App *a = app(engine());
			a->removeWindow(this);
		}
	}

	Container *Window::parent() {
		return myParent;
	}

	Frame *Window::rootFrame() {
		return myRoot;
	}

	Handle Window::handle() const {
		return myHandle;
	}

	void Window::attachParent(Container *parent) {
		myParent = parent;
		if (parent == this)
			myRoot = as<Frame>(this);
		else
			myRoot = parent->myRoot;
	}

	void Window::detachParent() {
		handle(invalid);
		myRoot = null;
	}

	void Window::windowDestroyed() {}

	void Window::parentCreated(nat id) {
		assert(myParent);

		myRoot = myParent->myRoot;

		if (!created())
			if (!create(myParent, id))
				WARNING(L"Failed to create a window...");
	}

	void Window::handle(Handle handle) {
		App *a = app(engine());
		if (myHandle != invalid) {
			notifyDetachPainter();
			if (myParent == null) {
				// We're a frame (or orphaned).
				destroyWindow(myHandle);
			} else if (!myParent->created()) {
				// Parent has already been destroyed. This means we're destroyed,
				// by our parent, even though we have a handle.
			} else {
				destroyWindow(myHandle);
			}

			// Notify we've been destroyed.
			myHandle = invalid;
			windowDestroyed();

			// DestroyWindow sends WM_CLOSE, which may be handled by this class.
			a->removeWindow(this);
		}

		myHandle = handle;
		if (myHandle != invalid) {
			a->addWindow(this);
			notifyAttachPainter();
		}
	}

	Bool Window::onKey(Bool down, key::Key id, Modifiers modifiers) {
		return false;
	}

	Bool Window::onChar(Nat id) {
		return false;
	}

	Bool Window::visible() {
		return myVisible;
	}

	Font *Window::font() {
		return new (this) Font(*myFont);
	}

	void Window::resized(Size size) {
		// Nothing here, override when needed.
	}

	void Window::painter(Painter *p) {
		notifyDetachPainter();

		if (myPainter) {
			if (!p)
				detachPainter();
		} else {
			if (p)
				attachPainter();
		}

		myPainter = p;

		if (created())
			notifyAttachPainter();
	}

	void Window::notifyAttachPainter() {
		if (myPainter) {
			Engine &e = engine();
			os::Future<void> result;
			Window *me = this;
			os::FnCall<void, 2> params = os::fnCall().add(myPainter).add(me);
			os::UThread::spawn(address(&Painter::attach), true, params, result, &Render::thread(e)->thread());
			result.result();
		}
	}

	void Window::notifyDetachPainter() {
		if (myPainter) {
			Engine &e = engine();
			os::Future<void> result;
			os::FnCall<void, 2> params = os::fnCall().add(myPainter);
			os::UThread::spawn(address(&Painter::detach), true, params, result, &Render::thread(e)->thread());
			result.result();
		}
	}

	void Window::notifyPainter(Size s) {
		if (myPainter) {
			Engine &e = engine();
			os::FnCall<void, 2> params = os::fnCall().add(myPainter).add(s);
			os::UThread::spawn(address(&Painter::resize), true, params, &Render::thread(e)->thread());
		}
	}

	void Window::onTimer() {}

#ifdef GUI_WIN32

	void Window::destroyWindow(Handle handle) {
		DestroyWindow(handle.hwnd());
	}

	MsgResult Window::onMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_SIZE: {
			Size s = pos().size();
			notifyPainter(s);
			resized(s);
			return msgResult(0);
		}
		case WM_PAINT:
			return onPaint();
		case WM_TIMER: {
			if (msg.wParam == 1) {
				onTimer();
				return msgResult(0);
			}
		}
		}
		return noResult();
	}

	MsgResult Window::beforeMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_KEYUP:
			if (onKey(false, keycode(msg.wParam), modifiers()))
				return msgResult(0);
			break;
		case WM_KEYDOWN:
			if (onKey(true, keycode(msg.wParam), modifiers()))
				return msgResult(0);
			break;
		case WM_CHAR:
			if (onChar(msg.wParam))
				return msgResult(0);
			break;
		}
		return noResult();
	}

	void Window::visible(Bool show) {
		if (created())
			ShowWindow(handle(), show ? TRUE : FALSE);
	}

	const Str *Window::text() {
		if (created()) {
			Nat len = GetWindowTextLength(handle());
			GcArray<wchar> *src = runtime::allocArray<wchar>(engine(), &wcharArrayType, len + 1);
			GetWindowText(handle(), src->v, len + 1);
			myText = (new (this) Str(src))->fromCrLf();
		}
		return myText;
	}

	void Window::text(Str *s) {
		myText = s;
		if (created())
			SetWindowText(handle(), s->toCrLf()->c_str());
	}

	Rect Window::pos() {
		if (created()) {
			RECT r;
			HWND h = handle();
			GetClientRect(h, &r);
			POINT a = { r.left, r.top };
			ClientToScreen(h, &a);
			POINT b = { r.right, r. bottom };
			ClientToScreen(h, &b);
			myPos = convert(a, b);
		}
		return myPos;
	}

	void Window::pos(Rect r) {
		myPos = r;
		if (created()) {
			RECT z = convert(r);
			HWND h = handle();
			DWORD style = GetWindowLong(h, GWL_STYLE);
			DWORD exStyle = GetWindowLong(h, GWL_EXSTYLE);
			AdjustWindowRectEx(&z, style, FALSE, exStyle);
			// Todo: keep track if we need to repaint.
			MoveWindow(handle(), z.left, z.top, z.right - z.left, z.bottom - z.top, TRUE);
		}
	}

	void Window::focus() {
		if (created())
			SetFocus(handle());
	}

	void Window::font(Font *font) {
		myFont = new (this) Font(*font);
		if (created())
			SendMessage(handle(), WM_SETFONT, (WPARAM)font->handle(), TRUE);
	}

	void Window::update() {
		if (created())
			UpdateWindow(handle());
	}

	void Window::repaint() {
		if (created())
			InvalidateRect(handle(), NULL, FALSE);
	}

	void Window::attachPainter() {}

	void Window::detachPainter() {}

	bool Window::onCommand(nat id) {
		return false;
	}

	bool Window::create(Window *container, nat id) {
		return createEx(NULL, childFlags, 0, parent->handle().hwnd(), id);
	}

	bool Window::createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent) {
		return createEx(className, style, exStyle, parent, 0, cNormal);
	}

	bool Window::createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent, nat id) {
		return createEx(className, style, exStyle, parent, id, cNormal);
	}

	bool Window::createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent, nat id, CreateFlags flags) {
		assert(handle() == invalid);

		App *app = gui::app(engine());

		if (className == null)
			className = (LPCTSTR)app->windowClass();

		if (cManualVisibility & ~flags)
			if (myVisible)
				style |= WS_VISIBLE;

		Point p = myPos.p0;
		Size s = myPos.size();

		if (WS_CHILD & ~style) {
			RECT r = convert(myPos);
			AdjustWindowRectEx(&r, style & ~WS_OVERLAPPED, FALSE, exStyle);
			Rect c = convert(r);
			p = c.p0;
			s = c.size();
		}

		if (flags & cAutoPos) {
			// Outside the screen? TODO: Check for something other than 0, screen coords may be negative!
			if (myPos.p1.x < 0 && myPos.p1.y < 0) {
				p.x = CW_USEDEFAULT;
				p.y = 0;
			}

			// Invalid size?
			if (!myPos.size().valid()) {
				s.w = CW_USEDEFAULT;
				s.h = 0;
			}
		}

		// Position controls before creation.
		if (myPos.size().valid())
			resized(myPos.size());

		HINSTANCE instance = app->instance();
		LPCTSTR windowName = myText->toCrLf()->c_str();

		app->preCreate(this);
		HWND z = CreateWindowEx(exStyle, className, windowName, style,
								(int)p.x, (int)p.y, (int)s.w, (int)s.h,
								parent, (HMENU)id, instance, NULL);

		if (z == NULL) {
			app->createAborted(this);
			return false;
		} else {
			handle(z);
			if (timerInterval.inMs() != 0) {
				setTimer(timerInterval);
			}
			SendMessage(handle(), WM_SETFONT, (WPARAM)myFont->handle(), TRUE);
			return true;
		}
	}

	MsgResult Window::onPaint() {
		if (myPainter) {
			Engine &e = engine();
			RepaintParams *param = null;
			os::Future<void> result;
			os::FnCall<void, 2> params = os::fnCall().add(myPainter).add(param);
			os::UThread::spawn(address(&Painter::repaint), true, params, result, &Render::thread(e)->thread());

			result.result();
			ValidateRect(handle(), NULL);

			return msgResult(0);
		} else {
			return noResult();
		}
	}

	void Window::setTimer(Duration interval) {
		timerInterval = interval;
		if (created()) {
			SetTimer(handle(), 1, (UINT)interval.inMs(), NULL);
		}
	}

	void Window::clearTimer() {
		KillTimer(handle(), 1);
	}

#endif
#ifdef GUI_GTK

	bool Window::create(Container *parent, nat id) {
		initWidget(parent, gtk_drawing_area_new());
		return true;
	}

	void Window::initWidget(Container *parent, GtkWidget *widget) {
		handle(widget);

		Size s = myPos.size();
		basic_put(parent->container(), widget, myPos.p0.x, myPos.p0.y, s.w, s.h);

		if (myVisible)
			gtk_widget_show(widget);
		else
			gtk_widget_hide(widget);

		if (myFont != app(engine())->defaultFont)
			gtk_widget_override_font(fontWidget(), myFont->desc());

		initSignals(widget);
	}

	void Window::initSignals(GtkWidget *widget) {
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onKeyDown>::to(widget, "key-press-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onKeyUp>::to(widget, "key-release-event", engine());
		Signal<void, Window, GdkRectangle *>::Connect<&Window::onSize>::to(widget, "size-allocate", engine());
		Signal<void, Window>::Connect<&Window::onRealize>::to(drawWidget(), "realize", engine());
		Signal<void, Window>::Connect<&Window::onUnrealize>::to(drawWidget(), "unrealize", engine());
		Signal<gboolean, Window, cairo_t *>::Connect<&Window::onDraw>::to(drawWidget(), "draw", engine());
	}

	void Window::destroyWindow(Handle handle) {
		gtk_widget_destroy(handle.widget());
	}

	gboolean Window::onKeyUp(GdkEvent *event) {
		GdkEventKey &k = event->key;
		return onKey(false, keycode(k), modifiers(k)) ? TRUE : FALSE;
	}

	gboolean Window::onKeyDown(GdkEvent *event) {
		GdkEventKey &k = event->key;
		bool ok = onKey(true, keycode(k), modifiers(k));
		if (ok)
			ok = onChar(k.keyval);
		return ok ? TRUE : FALSE;
	}

	void Window::onSize(GdkRectangle *alloc) {
		// If we have our own window, resize that as well.
		if (gdkWindow) {
			gdk_window_move_resize(gdkWindow, alloc->x, alloc->y, alloc->width, alloc->height);
		}

		Size s(alloc->width, alloc->height);
		notifyPainter(s);
		resized(s);
	}

	gboolean Window::onDraw(cairo_t *ctx) {
		// Do we have a painter?
		if (myPainter && gdkWindow) {
			Engine &e = engine();
			RepaintParams param = { gdkWindow, drawWidget() };
			RepaintParams *pParam = &param;
			os::Future<void> result;
			os::FnCall<void, 2> params = os::fnCall().add(myPainter).add(pParam);
			os::UThread::spawn(address(&Painter::repaint), true, params, result, &Render::thread(e)->thread());

			result.result();

			return TRUE;
		}

		// Do we have a window and need to paint the background?
		if (gdkWindow && gtk_cairo_should_draw_window(ctx, gdkWindow)) {
			GtkWidget *me = drawWidget();

			// Fill with the background color we figured out earlier.
			App *app = gui::app(engine());
			Color bg = app->defaultBgColor;
			cairo_set_source_rgba(ctx, bg.r, bg.g, bg.b, bg.a);
			cairo_paint(ctx);

			// It seems Gtk+ does not understand what we're doing and therefore miscalculates the x-
			// and y- positions of the layout during drawing. We correct this by using the offset in
			// the cairo_t.
			GtkAllocation position;
			gtk_widget_get_allocation(me, &position);

			cairo_save(ctx);
			cairo_translate(ctx, -position.x, -position.y);

			// Call the original draw function here, with our translated context.
			typedef gboolean (*DrawFn)(GtkWidget *, cairo_t *);
			DrawFn original = GTK_WIDGET_GET_CLASS(me)->draw;
			if (original)
				(*original)(me, ctx);

			cairo_restore(ctx);

			return TRUE;
		}

		return FALSE;
	}

	void Window::onRealize() {
		if (gdkWindow)
			return;

		// Check if we need to create a separate window for this widget. There are two cases in
		// which we need that:
		// 1: we have a painter attached to us
		// 2: our parent has a painter attached

		bool create = false;
		if (myPainter)
			create = true;
		if (myParent && myParent != this && myParent->myPainter)
			create = true;

		if (!create)
			return;

		// Create the window.
		GdkWindowAttr attrs;
		memset(&attrs, 0, sizeof(attrs));

		GtkWidget *drawTo = drawWidget();
		GdkWindow *parent = gtk_widget_get_window(drawTo);
		if (!parent)
			parent = gtk_widget_get_parent_window(drawTo);

		GtkAllocation alloc;
		gtk_widget_get_allocation(drawTo, &alloc);
		attrs.x = alloc.x;
		attrs.y = alloc.y;
		attrs.width = alloc.width;
		attrs.height = alloc.height;
		attrs.event_mask = gtk_widget_get_events(drawTo) | GDK_EXPOSURE_MASK;
		attrs.window_type = GDK_WINDOW_CHILD; // GDK_WINDOW_SUBSURFACE is nice on Wayland.
		attrs.wclass = GDK_INPUT_OUTPUT;
		// Probably good. We could use *_get_system_visual() instead.
		attrs.visual = gdk_screen_get_rgba_visual(gdk_window_get_screen(parent));

		gdkWindow = gdk_window_new(parent, &attrs, GDK_WA_X | GDK_WA_Y);
		gdk_window_ensure_native(gdkWindow);
		gdk_window_show_unraised(gdkWindow);

		gtk_widget_set_has_window(drawTo, true);
		gtk_widget_set_window(drawTo, gdkWindow);
		gdk_window_set_user_data(gdkWindow, drawTo);
		if (myPainter)
			gtk_widget_set_double_buffered(drawTo, false);
	}

	void Window::onUnrealize() {
		if (!gdkWindow)
			return;

		notifyDetachPainter();
		gdkWindow = null;
	}

	void Window::attachPainter() {
		if (!created())
			return;

		if (gdkWindow)
			return;

		// TODO: This does not work.
		GtkWidget *drawTo = drawWidget();
		gtk_widget_unrealize(drawTo);
		gtk_widget_realize(drawTo);
	}

	void Window::detachPainter() {
		if (!created())
			return;

		if (!gdkWindow)
			return;

		// TODO: This does not work, but can not happen currently.
		GtkWidget *drawTo = drawWidget();
		gtk_widget_unrealize(drawTo);
		gtk_widget_realize(drawTo);
	}

	const Str *Window::text() {
		return myText;
	}

	void Window::text(Str *str) {
		myText = str;
	}

	Rect Window::pos() {
		if (created()) {
			Size s = myPos.size();
			basic_move(parent()->container(), handle().widget(), myPos.p0.x, myPos.p0.y, s.w, s.h);
		}
		return myPos;
	}

	void Window::pos(Rect r) {
		myPos = r;
	}

	void Window::visible(Bool v) {
		if (v)
			gtk_widget_show(handle().widget());
		else
			gtk_widget_hide(handle().widget());
	}

	void Window::focus() {
		gtk_widget_grab_focus(handle().widget());
	}

	void Window::font(Font *font) {
		myFont = new (this) Font(*font);
		if (created())
			gtk_widget_override_font(fontWidget(), myFont->desc());
	}

	GtkWidget *Window::fontWidget() {
		return handle().widget();
	}

	GtkWidget *Window::drawWidget() {
		return handle().widget();
	}

	void Window::update() {
		if (created()) {
			GdkWindow *window = gtk_widget_get_window(handle().widget());
			if (window) {
				gdk_window_invalidate_rect(window, NULL, true);
				gdk_window_process_updates(window, true);
			}
		}
	}

	void Window::repaint() {
		if (created()) {
			GdkWindow *window = gtk_widget_get_window(handle().widget());
			if (window)
				gdk_window_invalidate_rect(window, NULL, true);
		}
	}

	void Window::setTimer(Duration interval) {
		timerInterval = interval;
		TODO(L"Implement timers!");
	}

	void Window::clearTimer() {
		TODO(L"Implement timers!");
	}

#endif

}
