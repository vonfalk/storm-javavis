#include "stdafx.h"
#include "App.h"
#include "LibData.h"
#include "Window.h"
#include "Frame.h"

namespace gui {

	App::App() : appWait(null), creating(null) {
		windows = new (this) Map<Handle, Window *>();
		liveWindows = new (this) Set<Window *>();
		defaultFont = gui::defaultFont(engine());

		init();
	}

	void App::terminate() {
		// Reap all live windows.
		for (WindowSet::Iter i = liveWindows->begin(), e = liveWindows->end(); i != e; ++i)
			i.v()->handle(Window::invalid);

		if (appWait) {
			os::Sema wait(0);
			appWait->terminate(wait);
			wait.down();
		}
	}

	void App::preCreate(Window *w) {
		assert(creating == null, L"Trying to create multiple windows simultaneously is not supported.");
		creating = w;
	}

	void App::createAborted(Window *w) {
		assert(creating == w, L"The specified window did not try to create a window.");
		creating = null;
	}

	void App::addWindow(Window *w) {
		if (creating == w)
			creating = null;

		windows->put(w->handle(), w);
		liveWindows->put(w);
	}

	void App::removeWindow(Window *w) {
		windows->remove(w->handle());
		liveWindows->remove(w);
	}

	Window *App::findWindow(Handle h) {
		return windows->get(h, null);
	}

	bool App::resumeEvent(Window *window, Event *event) {
		return liveWindows->has(window) == false
			|| event->isSet();
	}

	void App::waitForEvent(Window *owner, Event *event) {
		// Make sure not to block the message pumping UThread.
		if (os::UThread::current() != appWait->uThread) {
			event->wait();
			return;
		}

		// Keep the message pump running until the event is properly signaled!
		while (!resumeEvent(owner, event)) {
			do {
				appWait->enableMsg();
				appWait->work();
				appWait->disableMsg();
				if (resumeEvent(owner, event))
					break;
			} while (os::UThread::leave());

			if (resumeEvent(owner, event))
				break;

			appWait->enableMsg();
			os::Thread::current().threadData()->waitForWork();
			appWait->disableMsg();
			if (appWait->isDone())
				// Exiting message loop, return.
				return;
		}
	}

	App *app(EnginePtr e) {
		App *&v = appData(e.v);
		if (!v)
			v = new (e.v) App();
		return v;
	}


	/**
	 * Custom wait logic.
	 */

	AppWait::AppWait(Engine &e) : uThread(os::UThread::invalid), msgDisabled(0), e(e), notifyExit(null) {
		App *app = gui::app(e);
		app->appWait = this;
		done = false;
	}

	AppWait::~AppWait() {
		destroy();
	}

	void AppWait::disableMsg() {
		msgDisabled++;
	}

	void AppWait::enableMsg() {
		assert(msgDisabled > 0, L"You messed up with enable/disable.");
		if (msgDisabled > 0)
			msgDisabled--;
	}

	os::Thread spawnUiThread(Engine &e) {
		return os::Thread::spawn(new AppWait(e), runtime::threadGroup(e));
	}

	/**
	 * Platform specific interactions.
	 */
#ifdef GUI_WIN32

	// Current engine, used to retrieve the App for the current thread.
	static THREAD Engine *currentEngine = null;

	void App::init() {
		hInstance = GetModuleHandle(NULL);
		initCommonControls();
		hWindowClass = registerWindowClass();
	}

	ATOM App::windowClass() {
		return hWindowClass.atom();
	}

	HINSTANCE App::instance() {
		return hInstance.instance();
	}

	bool App::processMessages() {
		MSG msg;
		for (nat i = 0; i < maxProcessMessages; i++) {
			if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				break;

			if (msg.message == WM_QUIT)
				return false;

			// Just ignore these, there should not be too many of them anyway...
			if (msg.message == WM_THREAD_SIGNAL)
				continue;

			processMessage(msg);
		}

		return true;
	}

	void App::processMessage(MSG &msg) {
		if (Window *w = windows->get(Handle(msg.hwnd), null)) {
			// Intercepted?
			MsgResult r = w->beforeMessage(msg);
			if (r.any) {
				if (InSendMessage())
					ReplyMessage(r.result);
				return;
			}

			// Dialog message?
			Frame *root = w->rootFrame();
			if (root && IsDialogMessage(root->handle(), &msg))
				return;
		}

		// Translate and dispatch to window proc.
		TranslateMessage(&msg);
		currentEngine = &engine(); // Just to make sure!
		DispatchMessage(&msg);
	}

	MsgResult App::handleMessage(HWND hwnd, const Message &msg) {
		if (!currentEngine) {
			WARNING(L"No current engine. Ignoring " << msg << L".");
			return noResult();
		}

		App *app = gui::app(*currentEngine);
		Window *w = app->windows->get(Handle(hwnd), app->creating);
		if (!w) {
			WARNING(L"Unknown window: " << hwnd << L", ignoring " << msg << L".");
			return noResult();
		}

		// From here on, this thread may yeild, causing the main UThread to want to dispatch more
		// messages. Prevent this to avoid confusing the Win32 api by pre-empting calls on the stack
		// there.
		MsgResult r = noResult();
		app->appWait->disableMsg();

		try {
			r = w->onMessage(msg);
		} catch (const Exception &e) {
			PLN(L"Unhandled exception in window thread:\n" << e);
		}

		app->appWait->enableMsg();
		return r;
	}

	LRESULT WINAPI App::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		MsgResult r = handleMessage(hwnd, Message(msg, wParam, lParam));
		if (r.any)
			return r.result;

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	ATOM App::registerWindowClass() {
		static ATOM c = 0;

		// Someone already created our window class! (maybe this is bad due to different icons?)
		if (c)
			return c;

		WNDCLASSEX wc;
		zeroMem(wc);
		wc.cbSize = sizeof(WNDCLASSEX);
		TODO(L"Load icon from somewhere!");
		// HICON icon = LoadIcon(HINSTANCE, MAKEINTRESOURCE(iconResource));
		HICON icon = LoadIcon(NULL, IDI_APPLICATION);
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = &App::windowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance.instance();
		wc.hIcon = icon;
		wc.hIconSm = icon;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
		wc.lpszMenuName = 0;
		wc.lpszClassName = L"StormGui";

		// TODO: Multiple instances? Generate different names if this fails?
		c = RegisterClassEx(&wc);
		assert(c);
		return c;
	}

	void App::initCommonControls() {
		INITCOMMONCONTROLSEX cc;
		zeroMem(cc);
		cc.dwSize = sizeof(cc);
		cc.dwICC = ICC_WIN95_CLASSES | ICC_USEREX_CLASSES;
		InitCommonControlsEx(&cc);
	}


	void AppWait::init() {
		threadId = GetCurrentThreadId();
		uThread = os::UThread::current();
		signalSent = 0;
		currentEngine = &e;

		// Make sure we get a message queue.
		if (!IsGUIThread(TRUE)) {
			assert(false, L"Could not convert to a GUI thread.");
		}
	}

	void AppWait::destroy() {}

	bool AppWait::wait(os::IOHandle &io) {
		// Since we know the semantics of wait(IOHandle, nat), we can exploit that...
		return wait(io, INFINITE);
	}

	bool AppWait::wait(os::IOHandle &io, nat msTimeout) {
		if (done) {
			// No more need for message processing!
			return false;
		}

		if (msgDisabled) {
			if (msTimeout != INFINITE)
				fallback.wait(io, msTimeout);
			else
				fallback.wait(io);
		} else {
			HANDLE h = io.v();
			Nat count = h != NULL ? 1 : 0;
			MsgWaitForMultipleObjects(count, &h, FALSE, msTimeout, QS_ALLPOSTMESSAGE | QS_ALLINPUT);
			atomicWrite(signalSent, 0);
		}

		return !done;
	}

	void AppWait::signal() {
		// Only the first thread posts the message if needed.
		if (atomicCAS(signalSent, 0, 1) == 0)
			PostThreadMessage(threadId, WM_THREAD_SIGNAL, 0, 0);

		fallback.signal();
	}

	void AppWait::work() {
		try {
			// Do not handle messages if they are disabled.
			if (msgDisabled == 0) {
				App *app = gui::app(e);
				if (!app->processMessages()) {
					uThread = os::UThread::invalid;
					if (notifyExit)
						notifyExit->up();
					done = true;
				}

				// The function 'processMessages' consumes the WM_THREAD_SIGNAL message so if
				// someone wants to wake us up, they need to send a new message from here on. It is
				// safe to reset this flag here since we are going to let other threads run after
				// this point in the code anyway.
				atomicWrite(signalSent, 0);
			}
		} catch (const Exception &e) {
			PLN(L"Unhandled exception in window thread:\n" << e);
		}
	}

	void AppWait::terminate(os::Sema &notify) {
		notifyExit = &notify;
		if (done)
			notify.up();

		PostThreadMessage(threadId, WM_QUIT, 0, 0);
	}


#endif

#ifdef GUI_GTK

	void App::init() {
		// Nothing so far...
	}

	void AppWait::init() {
		done = false;
		// TODO? Pass 'standard' parameters from the command line somehow...
		gtk_init(NULL, NULL);

		// Find the context for the main loop.
		context = g_main_context_default();

		// Create the source for file descriptors.
		fdSource = g_source_new(&fdFuncs, sizeof(FdData));
		g_source_attach(fdSource, context);
	}

	void AppWait::destroy() {
		g_source_destroy(fdSource);
		g_source_unref(fdSource);
	}

	static short from_g(GIOCondition src) {
		short r = 0;
		if (src & G_IO_IN)
			r |= POLLIN;
		if (src & G_IO_OUT)
			r |= POLLOUT;
		if (src & G_IO_PRI)
			r |= POLLPRI;
		if (src & G_IO_ERR)
			r |= POLLERR;
		if (src & G_IO_HUP)
			r |= POLLHUP;
		if (src & G_IO_NVAL)
			r |= POLLNVAL;
		return r;
	}

	static GIOCondition to_g(short src) {
		GIOCondition r = GIOCondition(0);
		if (src & POLLIN)
			r = GIOCondition(r | G_IO_IN);
		if (src & POLLOUT)
			r = GIOCondition(r | G_IO_OUT);
		if (src & POLLPRI)
			r = GIOCondition(r | G_IO_PRI);
		if (src & POLLERR)
			r = GIOCondition(r | G_IO_ERR);
		if (src & POLLHUP)
			r = GIOCondition(r | G_IO_HUP);
		if (src & POLLNVAL)
			r = GIOCondition(r | G_IO_NVAL);
		return r;
	}

	GSourceFuncs AppWait::fdFuncs = {
		null, // prepare
		&AppWait::fdCheck, // check
		null, // dispatch
		null, // finalize
	};

	gboolean AppWait::fdCheck(GSource *source) {
		FdData *me = (FdData *)source;
		if (!me->fds)
			return FALSE;

		os::IOHandle::Desc &desc = *me->fds;
		bool any = false;

		for (size_t i = 1; i < desc.count; i++) {
			short result = from_g(g_source_query_unix_fd(source, me->tags[i]));
			desc.fds[i].revents = result;
			any |= result != 0;
		}

		if (any) {
			atomicWrite(*me->notify, 1);
			g_main_context_wakeup(me->context);
		}

		return FALSE;
	}

	void AppWait::doWait(os::IOHandle &io) {
		// Prepare waiting for the fds.
		FdData *fdData = (FdData *)fdSource;
		os::IOHandle::Desc desc = io.desc();
		tags.resize(desc.count); // Grow to fit.
		for (size_t i = 1; i < desc.count; i++)
			tags[i] = g_source_add_unix_fd(fdSource, desc.fds[i].fd, to_g(desc.fds[i].events));
		fdData->context = context;
		fdData->fds = &desc;
		fdData->tags = tags.data();
		fdData->notify = &signalSent;

		// Wait until we get an event. Perform more than one iteration if we have time.
		while (atomicRead(signalSent) == 0)
			g_main_context_iteration(context, TRUE);

		// Reset the 'events' variable here. If we reset it just before we enter the wile loop, we
		// might miss events from 'signal' if we are unlucky.
		atomicWrite(signalSent, 0);

		// Remove the file descriptors again.
		for (size_t i = 1; i < tags.size(); i++)
			g_source_remove_unix_fd(fdSource, tags[i]);
		fdData->fds = null;
		fdData->tags = null;
		fdData->notify = null;
	}

	bool AppWait::wait(os::IOHandle &io) {
		if (done)
			return false;

		if (msgDisabled) {
			fallback.wait(io);
		} else {
			doWait(io);
		}

		return !done;
	}

	gboolean AppWait::onTimeout(gpointer appWait) {
		AppWait *me = (AppWait *)appWait;
		atomicWrite(me->signalSent, 1);
		g_main_context_wakeup(me->context);
		return TRUE;
	}

	bool AppWait::wait(os::IOHandle &io, nat msTimeout) {
		if (done)
			return false;

		if (msgDisabled) {
			fallback.wait(io, msTimeout);
		} else {
			// Create a timeout.
			GSource *timeout = g_timeout_source_new(msTimeout);
			g_source_set_callback(timeout, &onTimeout, this, NULL);
			g_source_attach(timeout, context);

			// Wait for things to happen.
			doWait(io);

			// Destroy the timeout.
			g_source_destroy(timeout);
			g_source_unref(timeout);
		}

		return !done;
	}

	void AppWait::signal() {
		// Note: causes the next invocation of 'g_main_context_iteration' to return without blocking
		// if no thread is currently blocking inside 'g_main_context_iteration'. This makes it
		// behave like a Condition, which is what we want.
		if (atomicCAS(signalSent, 0, 1) == 0)
			g_main_context_wakeup(context);

		fallback.signal();
	}

	void AppWait::work() {
		// If we're already doing message processing, do not confuse Gtk by recursive calls to g_main_context_iteration.
		if (msgDisabled)
			return;

		// Try to dispatch a maximum of 100 events. Return as no event is dispatched so that we wait
		// properly instead of just burning CPU cycles for no good.
		bool dispatched = true;
		for (nat i = 0; i < App::maxProcessMessages && dispatched; i++) {
			dispatched &= g_main_context_iteration(context, FALSE) == TRUE;
		}
	}

	void AppWait::terminate(os::Sema &notify) {
		// We do not need to wait for the termination of the main loop. Just setting 'done' and
		// calling signal() is enough.
		notify.up();
		done = true;
		signal();
	}

#endif

}
