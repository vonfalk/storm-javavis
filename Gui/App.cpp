#include "stdafx.h"
#include "App.h"
#include "LibData.h"
#include "Window.h"
#include "Frame.h"
#include "Defaults.h"
#include "Core/Exception.h"
#include "Win32Dpi.h"

#ifdef POSIX
// NOTE: This does not exist on all POSIX systems (eg. MacOS)
#include <sys/eventfd.h>
#endif

namespace gui {

	Font *defaultFont(EnginePtr e) {
		return app(e)->defaultFont;
	}

	Color defaultBgColor(EnginePtr e) {
		return app(e)->defaultBgColor;
	}

	App::App() : appWait(null), creating(null) {
		windows = new (this) Map<Handle, Window *>();
		liveWindows = new (this) Set<Window *>();

		init();

		Defaults def = sysDefaults(engine());
		defaultFont = def.font;
		defaultBgColor = def.bgColor;
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

	Menu::Item *App::findMenuItem(Handle h) {
		for (WindowSet::Iter i = liveWindows->begin(), e = liveWindows->end(); i != e; ++i) {
			if (Frame *frame = as<Frame>(i.v()))
				if (Menu::Item *item = frame->findMenuItem(h))
					return item;
		}
		return null;
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
		if (!v) {
			// Try to be threadsafe.
			static util::Lock l;
			util::Lock::L z(l);
			if (!v) {
				// Poke the thread as well since we might be running on another OS thread.
				Ui::thread(e.v)->thread();

				v = new (e.v) App();
			}
		}
		return v;
	}


	/**
	 * Custom wait logic.
	 */

	AppWait::AppWait(Engine &e) : uThread(os::UThread::invalid), msgDisabled(0), e(e), notifyExit(null) {
		done = false;
	}

	void AppWait::init() {
		uThread = os::UThread::current();
		platformInit();
	}

	void AppWait::setup() {
		App *app = gui::app(e);
		app->appWait = this;
	}

	AppWait::~AppWait() {
		platformDestroy();
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
		setDpiAware();
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

			// Make sure we don't confuse the Win32 API by yeilding during this call.
			MsgResult r = noResult();
			appWait->disableMsg();
			try {
				r = w->beforeMessage(msg);
			} catch (...) {
				appWait->enableMsg();
				throw;
			}
			appWait->enableMsg();

			if (r.any) {
				if (InSendMessage())
					ReplyMessage(r.result);
				return;
			}

			// Dialog message?
			Frame *root = w->rootFrame();
			if (root && IsDialogMessage(root->handle().hwnd(), &msg))
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

		if (msg.msg == WM_NCCREATE) {
			// To properly scale menu bars etc. on systems that support it (not needed on later
			// versions of Windows 10, but earlier ones). We can't do this in the Frame itself
			// since it does not yet know the hwnd it has been assigned.
			enableNcScaling(hwnd);
			return noResult();
		}

		App *app = gui::app(*currentEngine);

		// Check for messages related to keeping the system interactive.
		if (!app->appWait->checkBlockMsg(hwnd, msg))
			return noResult();

		// Try to post it to a window.
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
		} catch (const storm::Exception *e) {
			PLN(L"Unhandled exception in window thread:\n" << e);
		} catch (const ::Exception &e) {
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

	void App::showDialog(bool show) {
		appWait->showDialog(show);
	}

	void AppWait::platformInit() {
		threadId = GetCurrentThreadId();
		signalSent = 0;
		currentEngine = &e;
		blockingDialogs = 0;
		blockTimer = NULL;

		// Make sure we get a message queue.
		if (!IsGUIThread(TRUE)) {
			assert(false, L"Could not convert to a GUI thread.");
		}
	}

	void AppWait::platformDestroy() {}

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
		// TODO: We want to ensure that the modal window loop entered when dragging the window (can
		// be detected by WM_ENTERSIZEMOVE or WM_EXITSIZEMOVE). The most common idea seems to be to
		// start a timer, and use that to see if we have other threads that wish to run.
		// See: http://www.tech-archive.net/Archive/Development/microsoft.public.win32.programmer.ui/2006-02/msg00153.html

		// We might need to complement this with hooks:
		// https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms644987(v=vs.85)

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
		} catch (const storm::Exception *e) {
			PLN(L"Unhandled exception in window thread:\n" << e);
		} catch (const ::Exception &e) {
			PLN(L"Unhandled exception in window thread:\n" << e);
		} catch (...) {
			PLN(L"Unhandled exception in window thread: <unknown>");
		}
	}

	void AppWait::terminate(os::Sema &notify) {
		notifyExit = &notify;
		if (done)
			notify.up();

		PostThreadMessage(threadId, WM_QUIT, 0, 0);
	}

	bool AppWait::checkBlockMsg(HWND hWnd, const Message &msg) {
		switch (msg.msg) {
		case WM_ENTERMENULOOP:
			// Called when a pop-up menu is started.
			blockStatus[hWnd] = blockMenu;
			break;
		case WM_EXITMENULOOP:
			// Called when we're out of the menu loop.
			blockStatus.erase(hWnd);
			break;
		case WM_ENTERSIZEMOVE:
			blockStatus[hWnd] = blockMoving;
			break;
		case WM_EXITSIZEMOVE:
			// We're always done when we get here!
			blockStatus.erase(hWnd);
			break;
		case WM_CAPTURECHANGED:
			// Seems to be a good "fallback" if the regular "done" notifications are not sent properly.
			blockStatus.erase(hWnd);
			break;
		case WM_DESTROY:
			// We don't need this information anymore.
			blockStatus.erase(hWnd);

			// If this is the window driving the timer, allocate a new timer.
			// Note: As long as we remove the timer, the code below will re-create it.
			if (blockTimer == hWnd) {
				KillTimer(hWnd, 0);
				blockTimer = NULL;
			}
			break;
		case WM_TIMER:
			if (msg.wParam == 0) {
				// This is our timer. It means we should check for UThreads that need to execute
				// now. This will, among other things, drive animations.
				os::UThread::leave();
				return false;
			}
		}

		if (blockStatus.empty()) {
			if (blockTimer != NULL) {
				KillTimer(blockTimer, 0);
				blockTimer = NULL;
			}
		} else {
			if (blockTimer == NULL) {
				blockTimer = hWnd;
				// This will generate a timeout of USER_TIMER_MINIMUM, which is ~10ms.
				// We use timer id 0 as that is not occupied by the window timer in the Window class.
				SetTimer(hWnd, 0, 1, NULL);
			}
		}

		// By default, we let messages through.
		return true;
	}

	void AppWait::showDialog(bool show) {
		if (show) {
			blockingDialogs++;
			// This will cause the next message to start the timer, which is enough.
			blockStatus[NULL] = blockModal;
		} else if (blockingDialogs > 1) {
			blockingDialogs--;
		} else {
			blockStatus.erase(NULL);
			if (blockTimer != NULL) {
				KillTimer(blockTimer, 0);
				blockTimer = NULL;
			}
		}
	}


#endif

#ifdef GUI_GTK

	void App::init() {
		// Note: App::init is called after AppWait::platformInit
		display = gdk_display_get_default();
	}

	void AppWait::platformInit() {
		done = false;
		dispatchReady = false;

		// We'll be using threads with X from time to time. Mainly while painting in the background through Cairo.
		XInitThreads();

		// Tell Gtk to not mess with the locale... This is not safe to do here, as we have multiple threads running!
		gtk_disable_setlocale();

		// TODO? Pass 'standard' parameters from the command line somehow...
		gtk_init(NULL, NULL);

		// Install our own event handler before the one Gtk+ installed. We need to intercept events
		// to windows where we are rendering using OpenGL.
		gdk_event_handler_set(&gtkEventHook, this, NULL);

		// Find the context for the main loop.
		context = g_main_context_default();
		// Become the owner of the main context.
		g_main_context_acquire(context);

		// We need an eventfd for our condition semantics.
		eventFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	}

	void AppWait::platformDestroy() {
		// Release ownership of the main context.
		g_main_context_release(context);

		// Close the eventfd.
		close(eventFd);
	}

	void AppWait::gtkEventHook(GdkEvent *event, gpointer data) {
		if (event->type == GDK_EXPOSE) {
			try {
				AppWait *me = (AppWait *)data;
				App *app = gui::app(me->e);

				// Find the Window associated with this event and pass a paint event directly to that
				// window before we let Gtk+ handle it and interfere with our OpenGL rendering.
				GtkWidget *widget = gtk_get_event_widget(event);
				Window *window = app->findWindow(widget);

				// Do not pass to Gtk+ if the window tells us not to.
				if (window && window->preExpose(widget))
					return;
			} catch (const storm::Exception *e) {
				PLN(L"Unhandled exception in window thread:\n" << e);
			} catch (const ::Exception &e) {
				PLN(L"Unhandled exception in window thread:\n" << e);
			} catch (...) {
				PLN(L"Unhandled exception in window thread: <unknown>");
			}
		}

		// Pass it on to Gtk+.
		gtk_main_do_event(event);
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

	static struct pollfd from_g(GPollFD fd) {
		struct pollfd r = {
			fd.fd,
			from_g(GIOCondition(fd.events)),
			from_g(GIOCondition(fd.revents))
		};
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

	static GPollFD to_g(struct pollfd fd) {
		GPollFD r = {
			fd.fd,
			to_g(fd.events),
			to_g(fd.revents)
		};
		return r;
	}

	void AppWait::plainWait(struct pollfd *fds, size_t count, int timeout) {
		int result = -1;
		while (result < 0) {
			result = poll(fds, count, timeout);

			if (result < 0) {
				if (errno != EINTR) {
					perror("poll");
					assert(false);
				}

				// TODO: Better approximation of the remaining time.
				if (timeout > 0)
					timeout = 0;
			}
		}
	}

	class NoSignals {
	public:
		NoSignals() {
			sigset_t block;
			sigemptyset(&old);
			sigfillset(&block);
			sigdelset(&block, SIGSEGV);
			pthread_sigmask(SIG_BLOCK, &block, &old);
		}

		~NoSignals() {
			pthread_sigmask(SIG_SETMASK, &old, NULL);
		}

	private:
		sigset_t old;
	};

	void AppWait::gtkWait(os::IOHandle::Desc &io, int stormTimeout) {
		// NOTE: We could check the return value of '_prepare' to possibly avoid polling, but the
		// return value is ignored in the Gtk+ implementation as well, so we can probably get away
		// with ignoring it as well.
		gint maxPriority = 0;
		gint timeout = 0;
		gint fdCount = 0;
		gint oldSize = 0;
		{
			// Block signals from the GC while calling g_main_context_prepare and
			// g_main_context_query. If signals arrive while Gtk+ is communicating with Xlib (mostly
			// during startup), it sometimes bails out when it sees the EINTR result from a system
			// call. It seems looks like the error is inside libxcb, which is used by libX11, but I am
			// not sure.
			// Note: it could be a very bad idea to block GC signals in this way since libxcb (or
			// libX11 for that matter) could try taking a lock that a sleeping thread is hogging at
			// the moment, which would cause a deadlock.
			NoSignals z;
			g_main_context_prepare(context, &maxPriority);

			// Get poll fd:s from Gtk+:
			do {
				oldSize = gPollFd.size();
				fdCount = g_main_context_query(context, maxPriority, &timeout, gPollFd.data(), oldSize);
				gPollFd.resize(fdCount);
			} while (fdCount > oldSize);
		}

		// Put them into an array of system specific poll fd:s.
		pollFd.resize(io.count + gPollFd.size());
		for (size_t i = 0; i < io.count; i++)
			pollFd[i] = io.fds[i];
		for (size_t i = 0; i < gPollFd.size(); i++)
			pollFd[i + io.count] = from_g(gPollFd[i]);

		// Adjust timeout.
		if (timeout < 0)
			timeout = stormTimeout;
		else if (stormTimeout >= 0)
			timeout = min(timeout, stormTimeout);

		// Now, we can call 'poll'!
		plainWait(pollFd.data(), pollFd.size(), timeout);

		// Copy the poll descriptors back to their original location...
		for (size_t i = 0; i < io.count; i++)
			io.fds[i] = pollFd[i];
		for (size_t i = 0; i < gPollFd.size(); i++)
			gPollFd[i] = to_g(pollFd[i + io.count]);

		{
			NoSignals z;
			// Let Gtk+ investigate the result of the polling.
			g_main_context_check(context, maxPriority, gPollFd.data(), gint(gPollFd.size()));
		}

		dispatchReady = true;
	}

	void AppWait::doWait(os::IOHandle &io, int timeout) {
		os::IOHandle::Desc desc = io.desc();
		desc.fds[0].fd = eventFd;
		desc.fds[0].events = POLLIN;
		desc.fds[0].revents = 0;

		if (msgDisabled) {
			plainWait(desc.fds, desc.count, timeout);
		} else {
			gtkWait(desc, timeout);
		}

		// If entry #0 is done, we want to read it so that it is not signaled anymore.
		if (desc.fds[0].revents != 0) {
			uint64_t v = 0;
			ssize_t r = read(eventFd, &v, 8);
			if (r <= 0)
				perror("Failed to read from eventfd");
		}

		// Notify that we woke up. This needs to be done after reading from the eventfd, otherwise
		// the 'signalSent' might be set again before we manage to clear the eventfd.
		atomicWrite(signalSent, 0);
	}

	bool AppWait::wait(os::IOHandle &io) {
		if (done)
			return false;

		doWait(io, -1);

		return !done;
	}

	bool AppWait::wait(os::IOHandle &io, nat msTimeout) {
		if (done)
			return false;

		msTimeout = min(msTimeout, nat(std::numeric_limits<int>::max()));
		doWait(io, msTimeout);

		return !done;
	}

	void AppWait::signal() {
		if (atomicCAS(signalSent, 0, 1) == 0) {
			uint64_t val = 1;
			while (true) {
				ssize_t r = write(eventFd, &val, 8);
				if (r >= 0)
					break;
				if (errno == EAGAIN || errno == EINTR)
					continue;
				perror("Failed to signal eventfd");
			}
		}
	}

	void AppWait::work() {
		// Don't try to dispatch anything unless we called wait earlier.
		if (!dispatchReady)
			return;

		// If we're already doing message processing, do not confuse Gtk by recursive calls to g_main_context_iteration.
		if (msgDisabled)
			return;

		msgDisabled = true;
		dispatchReady = false;

		// Dispatch any pending events.
		g_main_context_dispatch(context);

		msgDisabled = false;
	}

	void AppWait::terminate(os::Sema &notify) {
		uThread = os::UThread::invalid;

		// We do not need to wait for the termination of the main loop. Just setting 'done' and
		// calling signal() is enough.
		notify.up();
		done = true;
		signal();
	}

#endif

}
