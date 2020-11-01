#include "stdafx.h"
#include "Window.h"
#include "Container.h"
#include "Frame.h"
#include "App.h"
#include "Painter.h"
#include "Win32Dpi.h"
#include "GtkSignal.h"
#include "GtkEmpty.h"

// We're using gtk_widget_override_font, as there is no better way of setting custom fonts on widgets...
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace gui {

	// Null is a failure code from eg. GetParent function, should be OK to use as invalid.
	const Handle Window::invalid;

	Window::Window() :
		myHandle(invalid), myParent(null), myRoot(null),
		myVisible(true), drawing(false), mouseInside(false),
		gdkWindow(null), gTimer(null),
		myPos(0, 0, 40, 40) /* large enought to not generate warnings in Gtk+ */ {

		myText = new (this) Str(L"");
		myFont = app(engine())->defaultFont;
	}

	Window::~Window() {
		if (myHandle != invalid) {
			App *a = app(engine());
			a->removeWindow(this);
		}
	}

	ContainerBase *Window::parent() {
		return myParent;
	}

	Frame *Window::rootFrame() {
		return myRoot;
	}

	Handle Window::handle() const {
		return myHandle;
	}

	Size Window::minSize() {
		return Size();
	}

	void Window::attachParent(ContainerBase *parent) {
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
			if (myPainter)
				myPainter->onDetach();

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
			if (myPainter)
				myPainter->onAttach(this);
		}
	}

	Bool Window::onKey(Bool down, key::Key id, Modifiers modifiers) {
		return false;
	}

	Bool Window::onChar(Nat id) {
		return false;
	}

	Bool Window::onClick(Bool pressed, Point at, mouse::MouseButton button) {
		return false;
	}

	Bool Window::onDblClick(Point at, mouse::MouseButton button) {
		return false;
	}

	Bool Window::onMouseMove(Point at) {
		return false;
	}

	void Window::onMouseEnter() {}

	void Window::onMouseLeave() {}

	Bool Window::onMouseVScroll(Point at, Int delta) {
		return false;
	}

	Bool Window::onMouseHScroll(Point at, Int delta) {
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

	void Window::onResize(Size size) {
		resized(size);
	}

	void Window::painter(MAYBE(Painter *) p) {
		Painter *old = myPainter;
		myPainter = p;

		if (old) {
			old->onDetach();
			if (!p)
				detachPainter();
		} else {
			if (p)
				attachPainter();
		}

		if (created() && myPainter)
			myPainter->onAttach(this);
	}

	MAYBE(Painter *) Window::painter() {
		return myPainter;
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
			if (myPainter) {
				RECT r;
				GetClientRect(handle().hwnd(), &r);
				myPainter->onResize(Size(Float(r.right), Float(r.bottom)), dpiScale(currentDpi()));
			}
			onResize(s);
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
			// TODO: Propagate mouse messages to parent windows?
		case WM_LBUTTONDOWN:
			if (onClick(true, dpiFromPx(currentDpi(), mousePos(msg)), mouse::left))
				return msgResult(0);
			break;
		case WM_LBUTTONUP:
			if (onClick(false, dpiFromPx(currentDpi(), mousePos(msg)), mouse::left))
				return msgResult(0);
			break;
		case WM_LBUTTONDBLCLK:
			if (onDblClick(dpiFromPx(currentDpi(), mousePos(msg)), mouse::left))
				return msgResult(0);
			break;
		case WM_MBUTTONDOWN:
			if (onClick(true, dpiFromPx(currentDpi(), mousePos(msg)), mouse::middle))
				return msgResult(0);
			break;
		case WM_MBUTTONUP:
			if (onClick(false, dpiFromPx(currentDpi(), mousePos(msg)), mouse::middle))
				return msgResult(0);
			break;
		case WM_MBUTTONDBLCLK:
			if (onDblClick(dpiFromPx(currentDpi(), mousePos(msg)), mouse::middle))
				return msgResult(0);
			break;
		case WM_RBUTTONDOWN:
			if (onClick(true, dpiFromPx(currentDpi(), mousePos(msg)), mouse::right))
				return msgResult(0);
			break;
		case WM_RBUTTONUP:
			if (onClick(false, dpiFromPx(currentDpi(), mousePos(msg)), mouse::right))
				return msgResult(0);
			break;
		case WM_RBUTTONDBLCLK:
			if (onDblClick(dpiFromPx(currentDpi(), mousePos(msg)), mouse::right))
				return msgResult(0);
			break;
		case WM_MOUSEMOVE:
			if (!mouseInside) {
				TRACKMOUSEEVENT track;
				track.cbSize = sizeof(track);
				track.dwFlags = TME_LEAVE;
				track.hwndTrack = handle().hwnd();
				track.dwHoverTime = 0;
				TrackMouseEvent(&track);

				mouseInside = true;
				onMouseEnter();
			}

			if (onMouseMove(dpiFromPx(currentDpi(), mousePos(msg))))
				return msgResult(0);
			break;
		case WM_MOUSEWHEEL:
			if (onMouseVScroll(dpiFromPx(currentDpi(), mouseAbsPos(handle(), msg)), GET_WHEEL_DELTA_WPARAM(msg.wParam)))
				return msgResult(0);
			break;
		case WM_MOUSEHWHEEL:
			if (onMouseHScroll(dpiFromPx(currentDpi(), mouseAbsPos(handle(), msg)), GET_WHEEL_DELTA_WPARAM(msg.wParam)))
				return msgResult(0);
			break;
		case WM_MOUSELEAVE:
			mouseInside = false;
			onMouseLeave();
			break;
		}
		return noResult();
	}

	MsgResult Window::beforeMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (onKey(false, keycode(msg.wParam), modifiers()))
				return msgResult(0);
			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
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
		myVisible = show;
		if (created())
			ShowWindow(handle().hwnd(), show ? TRUE : FALSE);
	}

	Str *Window::text() {
		if (created()) {
			Nat len = GetWindowTextLength(handle().hwnd());
			GcArray<wchar> *src = runtime::allocArray<wchar>(engine(), &wcharArrayType, len + 1);
			GetWindowText(handle().hwnd(), src->v, len + 1);
			myText = (new (this) Str(src))->fromCrLf();
		}
		return myText;
	}

	void Window::text(Str *s) {
		myText = s;
		if (created())
			SetWindowText(handle().hwnd(), s->toCrLf()->c_str());
	}

	Rect Window::pos() {
		if (created()) {
			RECT r;
			HWND h = handle().hwnd();
			GetClientRect(h, &r);
			POINT a = { r.left, r.top };
			ClientToScreen(h, &a);
			POINT b = { r.right, r. bottom };
			ClientToScreen(h, &b);

			if (myParent != this) {
				ScreenToClient(myParent->handle().hwnd(), &a);
				ScreenToClient(myParent->handle().hwnd(), &b);
			}

			myPos = dpiFromPx(currentDpi(), convert(a, b));
		}
		return myPos;
	}

	void Window::pos(Rect r) {
		myPos = r;
		if (created()) {
			RECT z = convert(dpiToPx(currentDpi(), r));
			HWND h = handle().hwnd();
			DWORD style = GetWindowLong(h, GWL_STYLE);
			DWORD exStyle = GetWindowLong(h, GWL_EXSTYLE);
			AdjustWindowRectEx(&z, style, FALSE, exStyle);
			// TODO: keep track if we need to repaint.
			MoveWindow(h, z.left, z.top, z.right - z.left, z.bottom - z.top, TRUE);
		}
	}

	void Window::focus() {
		if (created())
			SetFocus(handle().hwnd());
	}

	void Window::font(Font *font) {
		if (font != myFont)
			myFont = new (this) Font(*font);
		if (created())
			SendMessage(handle().hwnd(), WM_SETFONT, (WPARAM)font->handle(currentDpi()), TRUE);
	}

	void Window::update() {
		if (created())
			UpdateWindow(handle().hwnd());
	}

	void Window::repaint() {
		if (created())
			InvalidateRect(handle().hwnd(), NULL, FALSE);
	}

	void Window::attachPainter() {
		if (!created())
			return;

		repaint();
	}

	void Window::detachPainter() {
		if (!created())
			return;

		// We must ask really hard to make Windows repaint the window properly.
		RedrawWindow(handle().hwnd(), NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
	}

	bool Window::onCommand(nat id) {
		return false;
	}

	Nat Window::currentDpi() {
		if (myRoot)
			return myRoot->currentDpi();
		else
			return defaultDpi;
	}

	void Window::updateDpi(Bool move) {
		// Update our position.
		if (move)
			pos(myPos);

		// Update the font.
		if (myFont)
			font(myFont);
	}

	bool Window::create(ContainerBase *parent, nat id) {
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

		if (cManualVisibility & ~flags) {
			if (myVisible) {
				style |= WS_VISIBLE;
			} else {
				style &= ~WS_VISIBLE;
			}
		}

		Point p = myPos.p0;
		Size s = myPos.size();

		if (WS_CHILD & ~style) {
			RECT r = convert(myPos);
			AdjustWindowRectEx(&r, style & ~WS_OVERLAPPED, FALSE, exStyle);
			Rect c = convert(r);
			p = c.p0;
			s = c.size();
		} else if (parent) {
			Nat dpi = windowDpi(parent);
			p = p * dpiScale(dpi);
			s = s * dpiScale(dpi);
		}

		// Put everything in ints now, so that we can properly set CW_USEDEFAULT without potential
		// rounding errors.
		int x = (int)p.x;
		int y = (int)p.y;
		int width = (int)s.w;
		int height = (int)s.h;

		if (flags & cAutoPos) {
			x = CW_USEDEFAULT;
			y = 0;

			if (parent != NULL) {
				// Center the dialog over the parent.
				RECT parentRect;
				GetWindowRect(parent, &parentRect);

				x = int(parentRect.left + (parentRect.right - parentRect.left) / 2 - s.w / 2);
				y = int(parentRect.top + (parentRect.bottom - parentRect.top) / 2 - s.h / 2);
			}

			// Invalid size?
			if (!myPos.size().valid()) {
				width = CW_USEDEFAULT;
				height = 0;
			}
		}

		// Position controls before creation.
		if (myPos.size().valid())
			onResize(myPos.size());

		HINSTANCE instance = app->instance();
		LPCTSTR windowName = myText->toCrLf()->c_str();

		app->preCreate(this);
		HWND z = CreateWindowEx(exStyle, className, windowName, style,
								x, y, width, height,
								parent, (HMENU)id, instance, NULL);

		if (z == NULL) {
			app->createAborted(this);
			return false;
		}

		if (WS_CHILD & ~style) {
			// Take DPI into account.
			Nat dpi = windowDpi(z);
			if (dpi != defaultDpi) {
				RECT r = convert(Rect(Point(), myPos.size() * dpiScale(dpi)));
				dpiAdjustWindowRectEx(&r, style & ~WS_OVERLAPPED, FALSE, exStyle, dpi);
				s = convert(r).size();
				SetWindowPos(z, NULL, 0, 0, int(s.w), int(s.h), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}

		handle(z);
		if (timerInterval != Duration()) {
			setTimer(timerInterval);
		}
		SendMessage(handle().hwnd(), WM_SETFONT, (WPARAM)myFont->handle(currentDpi()), TRUE);
		return true;
	}

	MsgResult Window::onPaint() {
		if (myPainter) {
			RepaintParams p;
			myPainter->onRepaint(&p);
			ValidateRect(handle().hwnd(), NULL);

			return msgResult(0);
		} else {
			return noResult();
		}
	}

	void Window::setTimer(Duration interval) {
		timerInterval = interval;
		if (created()) {
			// The HWND is used as a timer ID. That makes it unique. We can still use timer #0 for internal stuff.
			SetTimer(handle().hwnd(), (UINT_PTR)handle().hwnd(), (UINT)interval.inMs(), NULL);
		}
	}

	void Window::clearTimer() {
		KillTimer(handle().hwnd(), (UINT_PTR)handle().hwnd());
	}

#endif
#ifdef GUI_GTK

	class Timer {
	public:
		Timer(Duration interval, GtkWidget *widget, Engine &engine);
		~Timer();

	private:
		GtkWidget *widget;
		Engine *engine;
		guint id;

		static gboolean callback(gpointer user_data);
	};

	Timer::Timer(Duration interval, GtkWidget *widget, Engine &engine) :
		widget(widget), engine(&engine),
		id(g_timeout_add(interval.inMs(), &callback, this)) {}

	Timer::~Timer() {
		if (id) {
			GSource *s = g_main_context_find_source_by_id(NULL, id);
			if (s)
				g_source_destroy(s);

			id = 0;
		}
	}

	gboolean Timer::callback(gpointer user_data) {
		Timer *me = (Timer *)user_data;
		App *app = gui::app(*me->engine);

		Window *win = app->findWindow(Handle(me->widget));
		win->onTimer();

		return G_SOURCE_CONTINUE;
	}


	bool Window::create(ContainerBase *parent, nat id) {
		initWidget(parent, empty_new());
		return true;
	}

	void Window::initWidget(ContainerBase *parent, GtkWidget *widget) {
		handle(widget);

		parent->addChild(widget, myPos);

		if (myVisible)
			gtk_widget_show(widget);
		else
			gtk_widget_hide(widget);

		if (myFont != app(engine())->defaultFont)
			gtk_widget_override_font(fontWidget(), myFont->desc());

		initSignals(widget, drawWidget());
	}

	void Window::initSignals(GtkWidget *widget, GtkWidget *draw) {
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onKeyDown>::to(widget, "key-press-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onKeyUp>::to(widget, "key-release-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onButton>::to(widget, "button-press-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onButton>::to(widget, "button-release-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onMotion>::to(widget, "motion-notify-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onEnter>::to(widget, "enter-notify-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onLeave>::to(widget, "leave-notify-event", engine());
		Signal<gboolean, Window, GdkEvent *>::Connect<&Window::onScroll>::to(widget, "scroll-event", engine());
		Signal<void, Window, GdkRectangle *>::Connect<&Window::onSize>::to(widget, "size-allocate", engine());
		Signal<void, Window>::Connect<&Window::onRealize>::to(draw, "realize", engine());
		Signal<void, Window>::Connect<&Window::onUnrealize>::to(draw, "unrealize", engine());
		Signal<gboolean, Window, cairo_t *>::Connect<&Window::onDraw>::to(draw, "draw", engine());

		if (timerInterval != Duration())
			setTimer(timerInterval);
	}

	void Window::destroyWindow(Handle handle) {
		// TODO: Perhaps we should not call gtk_widget_destroy on all widgets. It seems it is enough
		// to destroy top-level widgets.
		gtk_widget_destroy(handle.widget());
		if (gTimer) {
			delete gTimer;
			gTimer = null;
		}
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

	// Translate widget coordinates to window coordinates.
	static void translateToWindow(GtkWidget *widget, gdouble &x, gdouble &y) {
		if (!widget)
			return;

		for (GtkWidget *at = widget; !gtk_widget_get_has_window(at); at = gtk_widget_get_parent(at)) {
			GtkAllocation delta;
			gtk_widget_get_allocation(at, &delta);

			x += delta.x;
			y += delta.y;
		}
	}

	// Translate window coordinates to widget coordinates.
	static bool translatePoint(GtkWidget *widget, gdouble x, gdouble y, Point &out) {
		GtkAllocation alloc;
		gtk_widget_get_allocation(widget, &alloc);

		// Traverse upwards until we find a widget with a window. That's where the event originated from!
		for (GtkWidget *at = widget; !gtk_widget_get_has_window(at); at = gtk_widget_get_parent(at)) {
			GtkAllocation delta;
			gtk_widget_get_allocation(at, &delta);

			x -= delta.x;
			y -= delta.y;
		}

		out.x = x;
		out.y = y;

		return out.x >= 0.0f && out.y >= 0.0f
			&& out.x < alloc.width && out.y < alloc.height;
	}

	gboolean Window::onButton(GdkEvent *event) {
		Point pt;
		GdkEventButton &b = event->button;
		if (!translatePoint(drawWidget(), b.x, b.y, pt))
			return FALSE;

		mouse::MouseButton button = mouse::MouseButton(b.button - 1);
		bool ok = false;

		if (button == mouse::left || button == mouse::middle || button == mouse::right) {
			switch (b.type) {
			case GDK_BUTTON_PRESS:
				ok = onClick(true, pt, button);
				break;
			case GDK_BUTTON_RELEASE:
				ok = onClick(false, pt, button);
				break;
			case GDK_2BUTTON_PRESS:
				ok = onDblClick(pt, button);
				break;
			}
		}
		return ok ? TRUE : FALSE;
	}

	gboolean Window::onMotion(GdkEvent *event) {
		Point pt;
		GdkEventMotion &m = event->motion;
		if (!translatePoint(drawWidget(), m.x, m.y, pt))
			return FALSE;

		return onMouseMove(pt) ? TRUE : FALSE;
	}

	gboolean Window::onEnter(GdkEvent *) {
		onMouseEnter();

		// No propagation.
		return TRUE;
	}

	gboolean Window::onLeave(GdkEvent *) {
		onMouseLeave();

		// No propagation.
		return TRUE;
	}

	gboolean Window::onScroll(GdkEvent *event) {
		Point pt;
		GdkEventScroll &s = event->scroll;
		if (!translatePoint(drawWidget(), s.x, s.y, pt))
			return FALSE;

		gdouble dx = 0, dy = 0;
		if (!gdk_event_get_scroll_deltas(event, &dx, &dy)) {
			dx = 120; dy = 120;
		}

		bool ok = false;
		switch (s.direction) {
		case GDK_SCROLL_UP:
			ok = onMouseVScroll(pt, Int(dx));
			break;
		case GDK_SCROLL_DOWN:
			ok = onMouseVScroll(pt, -Int(dx));
			break;
		case GDK_SCROLL_LEFT:
			ok = onMouseHScroll(pt, -Int(dy));
			break;
		case GDK_SCROLL_RIGHT:
			ok = onMouseHScroll(pt, Int(dy));
			break;
		}
		return ok ? TRUE : FALSE;
	}

	void Window::onSize(GdkRectangle *alloc) {
		// Note: We're interested in forwarding the size of the drawing widget (if it differs from
		// the root-widget of the window).
		GtkWidget *draw = drawWidget();
		GtkAllocation drawAlloc;
		gtk_widget_get_allocation(draw, &drawAlloc);

		// If we have our own window, resize that as well.
		if (gdkWindow) {
			// Modify the x and y coords if we're several layers down.
			gdouble x = drawAlloc.x, y = drawAlloc.y;
			translateToWindow(gtk_widget_get_parent(draw), x, y);

			// gdk_window_move_resize(gdkWindow, alloc->x, alloc->y, alloc->width, alloc->height);
			gdk_window_move_resize(gdkWindow, x, y, drawAlloc.width, drawAlloc.height);
		}

		Size s(drawAlloc.width, drawAlloc.height);
		myPos.size(s);

		if (myPainter)
			myPainter->onResize(s, 1);
		onResize(s);
	}

	bool Window::preExpose(GtkWidget *widget) {
		// This is where we can hook into paint events before Gtk+ gets hold of them!

		// Do we have a painter that is using the raw mode?
		if (myPainter && myPainter->getDeviceType() == dtRaw) {
			if (gdkWindow && widget == drawWidget()) {
				RepaintParams params = { gdkWindow, drawWidget(), NULL };
				myPainter->onRepaint(&params);
				return true;
			}
		}

		// Process normally.
		return false;
	}

	gboolean Window::onDraw(cairo_t *ctx) {
		GdkWindow *window = gdkWindow;
		if (!window)
			window = gtk_widget_get_window(drawWidget());

		// Anything we should do at all?
		if (!window)
			return FALSE;

		// Is this draw event for us?
		if (!gtk_cairo_should_draw_window(ctx, window))
			return FALSE;

		// We will call ourselves in some cases.
		if (drawing)
			return FALSE;

		// Do we have a painter?
		if (myPainter) {
			RepaintParams params = { window, drawWidget(), ctx };
			myPainter->onRepaint(&params);

			if (GDK_IS_WAYLAND_WINDOW(window)) {
				// This is a variant of what we're doing below.
				GtkWidget *me = drawWidget();
				GtkWidget *parentWidget = gtk_widget_get_parent(me);
				if (!parentWidget || !GTK_IS_CONTAINER(parentWidget)) {
					// If no parent exists, or the parent is not a container, we simply delegate to the
					// default behaviour. This should not happen in practice, but it is good to have a
					// decent fallback. Since the parent is not a container, the default implementation
					// will probably work anyway.
					return false;
				}

				GtkContainer *parent = GTK_CONTAINER(gtk_widget_get_parent(me));

				// It seems Gtk+ does not understand what we're doing and therefore miscalculates the x-
				// and y- positions of the layout during drawing. We correct this by using the offset in
				// the cairo_t.
				GtkAllocation position;
				gtk_widget_get_allocation(me, &position);

				// Since we have to pretend that the current child does not have a window when calling
				// 'gtk_container_propagate_draw' below, the current widget's offset will be applied twice.
				cairo_translate(ctx, -position.x, -position.y);

				drawing = true;
				gtk_container_propagate_draw(parent, me, ctx);
				drawing = false;
			}

			return TRUE;
		}

		// Do we have our own window and need to paint the background?
		if (!gdkWindow)
			return FALSE;

		// Do we have a window and need to paint the background?
		GtkWidget *me = drawWidget();
		GtkWidget *parentWidget = gtk_widget_get_parent(me);
		if (!parentWidget || !GTK_IS_CONTAINER(parentWidget)) {
			// If no parent exists, or the parent is not a container, we simply delegate to the
			// default behaviour. This should not happen in practice, but it is good to have a
			// decent fallback. Since the parent is not a container, the default implementation
			// will probably work anyway.
			return FALSE;
		}
		GtkContainer *parent = GTK_CONTAINER(gtk_widget_get_parent(me));


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

		// Since we have to pretend that the current child does not have a window when calling
		// 'gtk_container_propagate_draw' below, the current widget's offset will be applied twice.
		cairo_translate(ctx, -position.x*2, -position.y*2);

		// Call the original draw function here, with our translated context.
		// typedef gboolean (*DrawFn)(GtkWidget *, cairo_t *);
		// DrawFn original = GTK_WIDGET_GET_CLASS(me)->draw;
		// if (original)
		// 	(*original)(me, ctx);

		// Call 'gtk_container_propagate_draw' to draw ourselves again. Remember that we are
		// currently drawing so that we do not get into an infinite recursion when we are called
		// again. We could call 'GTK_WIDGET_GET_CLASS(me)->draw' instead, but that function
		// assumes that the cairo_t is properly transformed so that (0, 0) is at the top left of
		// the widget. That is not the case now, due to the possible bug inside
		// 'gtk_container_propagate_draw'.
		gtk_widget_set_has_window(me, FALSE);
		drawing = true;
		gtk_container_propagate_draw(parent, me, ctx);
		drawing = false;
		gtk_widget_set_has_window(me, TRUE);

		return TRUE;
	}

	bool Window::useNativeWindow() {
		// On Wayland, we don't need a separate window for things we're rendering into. That only
		// confuses the window manager!
		// TODO: Can we do this on X as well when we're using a blitting graphics backend?
		if (GDK_IS_WAYLAND_WINDOW(gtk_widget_get_window(drawWidget()))) {
			return false;
		}

		// Check if we need to create a separate window for this widget. There are two cases in
		// which we need that:
		// 1: we have a painter attached to us
		// 2: our parent has a painter attached

		if (myPainter)
			return true;
		if (myParent && myParent != this && myParent->myPainter)
			return true;
		return false;
	}

	void Window::onRealize() {
		if (gdkWindow)
			return;

		if (!useNativeWindow()) {
			setWindowMask(gtk_widget_get_window(drawWidget()));
			return;
		}

		// Create the window.
		GdkWindowAttr attrs;
		memset(&attrs, 0, sizeof(attrs));

		GtkWidget *drawTo = drawWidget();

		GdkWindow *oldWindow = gtk_widget_get_window(drawTo);
		GdkWindow *parentWindow = gtk_widget_get_parent_window(drawTo);
		if (oldWindow && parentWindow != oldWindow) {
			WARNING(L"Using previously created windows could be bad.");
			// This widget already has its own window. Use that.
			gdkWindow = oldWindow;
			if (myPainter)
				gtk_widget_set_double_buffered(drawTo, FALSE);
			return;
		}
		if (!oldWindow)
			oldWindow = parentWindow;

		GtkAllocation alloc;
		gtk_widget_get_allocation(drawTo, &alloc);
		gdouble x = 0, y = 0;
		translateToWindow(drawTo, x, y);
		attrs.x = x;
		attrs.y = y;
		attrs.width = alloc.width;
		attrs.height = alloc.height;
		attrs.event_mask = gtk_widget_get_events(drawTo) | GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
		attrs.window_type = GDK_WINDOW_CHILD; // GDK_WINDOW_SUBSURFACE is nice on Wayland.
		attrs.wclass = GDK_INPUT_OUTPUT;
		// Probably good. We could use *_get_system_visual() instead.
		attrs.visual = gdk_screen_get_rgba_visual(gdk_window_get_screen(oldWindow));

		gdkWindow = gdk_window_new(oldWindow, &attrs, GDK_WA_X | GDK_WA_Y);
		gdk_window_ensure_native(gdkWindow);
		gdk_window_show_unraised(gdkWindow);

		// Unref the old window.
		if (oldWindow)
			g_object_unref(oldWindow);

		gtk_widget_set_has_window(drawTo, TRUE);
		gtk_widget_set_window(drawTo, gdkWindow);
		gdk_window_set_user_data(gdkWindow, drawTo);
		if (myPainter) {
			gtk_widget_set_double_buffered(drawTo, FALSE);
		}
		setWindowMask(gdkWindow);
	}

	void Window::onUnrealize() {
		if (!gdkWindow)
			return;

		GtkWidget *drawTo = drawWidget();
		// gtk_widget_set_window(drawTo, NULL);
		gtk_widget_set_has_window(drawTo, FALSE);

		if (myPainter)
			myPainter->onDetach();
		gdkWindow = null;
	}

	void Window::setWindowMask(GdkWindow *window) {
		if (window) {
			int mask = gdk_window_get_events(window);
			mask |= GDK_POINTER_MOTION_MASK;
			mask |= GDK_BUTTON_MOTION_MASK;
			mask |= GDK_BUTTON_PRESS_MASK;
			mask |= GDK_BUTTON_RELEASE_MASK;
			mask |= GDK_SCROLL_MASK;
			mask |= GDK_SMOOTH_SCROLL_MASK;
			mask |= GDK_LEAVE_NOTIFY_MASK;
			gdk_window_set_events(window, GdkEventMask(mask));
		}
	}

	static void recreate_widget(GtkWidget *widget) {
		gtk_widget_unmap(widget);
		gtk_widget_unrealize(widget);

		gtk_widget_realize(widget);
		gtk_widget_map(widget);
	}

	void Window::attachPainter() {
		if (!created())
			return;

		if (gdkWindow)
			return;

		recreate_widget(drawWidget());
	}

	void Window::detachPainter() {
		if (!created())
			return;

		if (!gdkWindow)
			return;

		recreate_widget(drawWidget());
	}

	Str *Window::text() {
		return myText;
	}

	void Window::text(Str *str) {
		myText = str;
	}

	Rect Window::pos() {
		return myPos;
	}

	void Window::pos(Rect r) {
		myPos = r;
		if (created() && parent()) {
			parent()->moveChild(handle().widget(), myPos);
		}
	}

	void Window::visible(Bool v) {
		myVisible = v;
		if (created()) {
			if (v)
				gtk_widget_show(handle().widget());
			else
				gtk_widget_hide(handle().widget());
		}
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
		if (created()) {
			delete gTimer;
			gTimer = null;

			if (interval == Duration())
				return;

			gTimer = new Timer(interval, handle().widget(), engine());
		}
	}

	void Window::clearTimer() {
		if (gTimer)
			delete gTimer;
		gTimer = null;
	}

#endif

}
