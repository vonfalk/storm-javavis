#include "stdafx.h"
#include "App.h"
#include "LibData.h"
#include "Window.h"
#include "Frame.h"

namespace gui {

	// Current engine, used to retrieve the App for the current thread.
	static THREAD Engine *currentEngine = null;

	App::App() : appWait(null), creating(null) {
		windows = new (this) Map<Handle, Window *>();
		liveWindows = new (this) Set<Window *>();
		hInstance = GetModuleHandle(NULL);
		initCommonControls();
		hWindowClass = registerWindowClass();

		defaultFont = gui::defaultFont(engine());
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

	ATOM App::windowClass() {
		return hWindowClass.atom();
	}

	HINSTANCE App::instance() {
		return hInstance.instance();
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

		HWND handle = w->handle();
		windows->put(Handle(handle), w);
		liveWindows->put(w);
	}

	void App::removeWindow(Window *w) {
		windows->remove(Handle(w->handle()));
		liveWindows->remove(w);
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

	bool AppWait::wait(os::IOHandle io) {
		// Since we know the semantics of wait(IOHandle, nat), we can exploit that...
		return wait(io, INFINITE);
	}

	bool AppWait::wait(os::IOHandle io, nat msTimeout) {
		if (done) {
			// No more need for message processing!
			return false;
		}

		if (msgDisabled) {
			if (msTimeout != INFINITE)
				fallback.wait(msTimeout);
			else
				fallback.wait();
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

}
