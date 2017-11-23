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

	Window::Window() : myHandle(invalid), myParent(null), myRoot(null), myVisible(true), myPos(0, 0, 10, 10) {
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
			attachPainter();
		} else {
			detachPainter();
		}
	}

	Bool Window::onKey(Bool down, Nat id, Modifiers modifiers) {
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
		detachPainter();

		myPainter = p;

		if (created())
			attachPainter();
	}

	void Window::attachPainter() {
		if (myPainter) {
			Engine &e = engine();
			os::Future<void> result;
			Window *me = this;
			os::FnCall<void, 2> params = os::fnCall().add(myPainter).add(me);
			os::UThread::spawn(address(&Painter::attach), true, params, result, &Render::thread(e)->thread());
			result.result();
		}
	}

	void Window::detachPainter() {
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
			if (onKey(false, msg.wParam, modifiers()))
				return msgResult(0);
			break;
		case WM_KEYDOWN:
			if (onKey(true, msg.wParam, modifiers()))
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
			os::Future<void> result;
			os::FnCall<void, 1> params = os::fnCall().add(myPainter);
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

	gboolean Window::onKeyUp(GdkEvent *event) {
		GdkEventKey &k = event->key;
		return onKey(false, k.keyval, modifiers(k.state)) == TRUE;
	}

	gboolean Window::onKeyDown(GdkEvent *event) {
		GdkEventKey &k = event->key;
		return onKey(false, k.keyval, modifiers(k.state)) == TRUE;
	}

	void Window::onSize(GdkRectangle *alloc) {
		resized(Size(alloc->width, alloc->height));
	}

	gboolean Window::onDraw(cairo_t *ctx) {
		return FALSE;
		// TODO: We need to draw any child windows as well when returning 'true'.
		// int w = gtk_widget_get_allocated_width(handle().widget());
		// int h = gtk_widget_get_allocated_height(handle().widget());
		// PVAR(w);
		// PVAR(h);

		// cairo_set_source_rgb(ctx, 100, 0, 0);
		// cairo_set_line_width(ctx, 10);
		// cairo_rectangle(ctx, 0, 0, 100, 100);
		// cairo_fill(ctx);

		// return TRUE;
	}

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
		Signal<gboolean, Window, cairo_t *>::Connect<&Window::onDraw>::to(widget, "draw", engine());
	}

	void Window::destroyWindow(Handle handle) {
		gtk_widget_destroy(handle.widget());
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
	}

	void Window::clearTimer() {
	}

#endif

}
