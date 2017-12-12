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
		myVisible(true), renderWindow(null), myPos(0, 0, 10, 10) {

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
			notifyAttachPainter();
		} else {
			notifyDetachPainter();
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
		Signal<void, Window>::Connect<&Window::onRealize>::to(widget, "realize", engine());
		Signal<void, Window>::Connect<&Window::onUnrealize>::to(widget, "unrealize", engine());
		Signal<gboolean, Window, cairo_t *>::Connect<&Window::onDraw>::to(drawWidget(), "draw", engine());
	}

	void Window::destroyWindow(Handle handle) {
		gtk_widget_destroy(handle.widget());
	}

	gboolean Window::onKeyUp(GdkEvent *event) {
		GdkEventKey &k = event->key;
		return onKey(false, k.keyval, modifiers(k.state)) ? TRUE : FALSE;
	}

	gboolean Window::onKeyDown(GdkEvent *event) {
		GdkEventKey &k = event->key;
		return onKey(true, k.keyval, modifiers(k.state)) ? TRUE : FALSE;
	}

	void Window::onSize(GdkRectangle *alloc) {
		// If we have our own window, resize that as well.
		if (renderWindow) {
			PVAR(alloc->x); PVAR(alloc->y); PVAR(alloc->width); PVAR(alloc->height);
			gdk_window_move_resize(renderWindow, alloc->x, alloc->y, alloc->width, alloc->height);
		}

		Size s(alloc->width, alloc->height);
		notifyPainter(s);
		resized(s);
	}

	gboolean Window::onDraw(cairo_t *ctx) {
		if (myPainter) {
			if (renderWindow) {
				// Testing the rendering:
				static GlContext *c = null;
				if (!c)
					c = GlContext::create(renderWindow);

				c->activate();
				glClearColor(0, 1, 0, 0.5);
				glClear(GL_COLOR_BUFFER_BIT);
				c->swapBuffers();
			}
			return TRUE;


			Engine &e = engine();
			RepaintParams param = { ctx, handle().widget() };
			RepaintParams *pParam = &param;
			os::Future<void> result;
			os::FnCall<void, 2> params = os::fnCall().add(myPainter).add(pParam);
			os::UThread::spawn(address(&Painter::repaint), true, params, result, &Render::thread(e)->thread());

			result.result();
		}

		return FALSE;
	}

	void Window::onRealize() {
		if (!myPainter)
			return;

		if (renderWindow)
			return;

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

		renderWindow = gdk_window_new(parent, &attrs, GDK_WA_X | GDK_WA_Y);
		gdk_window_ensure_native(renderWindow);
		gdk_window_show_unraised(renderWindow);

		// gtk_widget_set_has_window(drawTo, true);
		gtk_widget_set_window(drawTo, renderWindow);
		gtk_widget_set_double_buffered(drawTo, false);
		return;

		// TODO: Move to RenderMgr.
		::Window window = GDK_WINDOW_XID(renderWindow);
		Display *xDisplay = GDK_DISPLAY_XDISPLAY(gdk_window_get_display(renderWindow));

		int major, minor;
		glXQueryVersion(xDisplay, &major, &minor);
		PVAR(major); PVAR(minor);

		GlContext::create(renderWindow);
		return;

		// Note: Minor should be larger than 2 for FBConfig to work...
		int visualAttrs[] = {
			GLX_X_RENDERABLE    , True,
			GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
			GLX_RENDER_TYPE     , GLX_RGBA_BIT,
			GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
			GLX_RED_SIZE        , 8,
			GLX_GREEN_SIZE      , 8,
			GLX_BLUE_SIZE       , 8,
			//GLX_ALPHA_SIZE      , 8,
			GLX_DEPTH_SIZE      , 24,
			GLX_STENCIL_SIZE    , 8,
			GLX_DOUBLEBUFFER    , True,
			//GLX_SAMPLE_BUFFERS  , 1,
			//GLX_SAMPLES         , 4,
			None
		};

		int fbcount = 0;
		GLXFBConfig *fbc = glXChooseFBConfig(xDisplay, DefaultScreen(xDisplay), visualAttrs, &fbcount);
		assert(fbcount > 0, L"No available formats for the frame buffer.");

		// Pick best type:
		int bestId = 0;
		int bestSamples = 0;
		for (int i = 0; i < fbcount; i++) {
			int buffers = 0, samples = 0;
			glXGetFBConfigAttrib(xDisplay, fbc[i], GLX_SAMPLE_BUFFERS, &buffers);
			glXGetFBConfigAttrib(xDisplay, fbc[i], GLX_SAMPLES, &samples);

			if (buffers >= 0 && samples > bestSamples) {
				bestId = i;
				bestSamples = 0;
			}
		}

		XVisualInfo *vi = glXGetVisualFromFBConfig(xDisplay, fbc[bestId]);
		PLN(L"Best format:");
		PVAR(vi->visualid);
		PVAR(vi->depth);
		PVAR(vi->colormap_size);
		PVAR(vi->bits_per_rgb);
		XFree(vi);

		// TODO: Set error handler here...
		GLXContext ctx = glXCreateNewContext(xDisplay, fbc[bestId], GLX_RGBA_TYPE, 0, True);
		XFree(fbc);

		PVAR(window);
		PVAR(ctx);
		PVAR(glXIsDirect(xDisplay, ctx));

		PVAR(glXMakeCurrent(xDisplay, window, ctx));
		glClearColor(0, 0.5, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		// NOTE: Does not block until VBlank; the next gl command does, however.
		glXSwapBuffers(xDisplay, window);
	}

	void Window::onUnrealize() {
		renderWindow = null;
	}

	void Window::attachPainter() {
		if (!created())
			return;

		if (renderWindow)
			return;

		// TODO: This does not work.
		GtkWidget *drawTo = drawWidget();
		gtk_widget_unrealize(drawTo);
		gtk_widget_realize(drawTo);
	}

	void Window::detachPainter() {
		if (!created())
			return;

		if (!renderWindow)
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
