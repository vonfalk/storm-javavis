#include "stdafx.h"
#include "App.h"
#include "Window.h"
#include "Frame.h"
#include <Objbase.h>


namespace stormgui {

	// The app object owned by this thread. Updated by 'processMessage'.
	static THREAD App *currentApp = null;

	App::App() : appWait(null), creating(null) {
		// TODO: Is this ok in our case? This returns the HINSTANCE of the exe-file, but that is
		// probably what we want in this case.
		hInstance = GetModuleHandle(NULL);
		initCommonControls();
		initCom();
		hWindowClass = registerWindowClass();

		defaultFont = stormgui::defaultFont(engine());
	}

	App::~App() {
		TODO(L"Reap any live windows!");

		CoUninitialize();
	}

	ATOM App::windowClass() {
		return hWindowClass;
	}

	HINSTANCE App::instance() {
		return hInstance;
	}

	bool App::resumeEvent(Window *window, os::Event &e) {
		return liveWindows.count(window) == 0 || e.isSet();
	}

	void App::waitForEvent(Window *window, os::Event &e) {
		if (os::UThread::current() != appWait->uThread) {
			e.wait();
			return;
		}

		// We need to run the message loop while checking the event.
		while (!resumeEvent(window, e)) {
			do {
				appWait->work();
				if (resumeEvent(window, e))
					return;
			} while (os::UThread::leave());

			if (resumeEvent(window, e))
				return;

			if (!appWait->wait())
				// Exiting message loop, return.
				return;
		}
	}

	void App::preCreate(Window *w) {
		assert(creating == null, L"Trying to create multiple windows simultaneously is not yet supported.");
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

		windows.insert(make_pair(handle, w));
		liveWindows.insert(w);
	}

	void App::removeWindow(Window *w) {
		windows.erase(w->handle());
		liveWindows.erase(w);
	}

	void App::terminate() {
		appWait->terminate();
	}

	bool App::processMessages() {
		MSG msg;

		for (nat i = 0; i < maxProcessMessages; i++) {
			if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				break;

			if (msg.message == WM_QUIT)
				return false;

			// Just ignore these, there should not be too many of them...
			if (msg.message == WM_THREAD_SIGNAL)
				continue;

			processMessage(msg);
		}

		return true;
	}

	void App::processMessage(MSG &msg) {
		WindowMap::iterator i = windows.find(msg.hwnd);
		if (i != windows.end()) {
			Window *w = i->second;

			// Intercepted?
			MsgResult r = w->beforeMessage(msg);
			if (r.any) {
				if (InSendMessage())
					ReplyMessage(r.result);
				return;
			}

			// Dialog message?
			Auto<Frame> root = w->rootFrame();
			if (root && IsDialogMessage(root->handle(), &msg))
				return;
		}

		// Translate and dispatch to Window Proc.
		TranslateMessage(&msg);
		currentApp = this; // Just to make sure!
		DispatchMessage(&msg);
	}

	MsgResult App::handleMessage(HWND hwnd, const Message &msg) {
		App *app = currentApp;
		if (!app) {
			WARNING(L"No current app. Ignoring " << msg << L".");
			return noResult();
		}

		Window *w = app->creating;
		WindowMap::iterator i = app->windows.find(hwnd);
		if (i != app->windows.end())
			w = i->second;

		if (w == null) {
			WARNING(L"Unknwon window: " << hwnd << L", ignoring " << msg << L".");
			return noResult();
		}

		try {
			return w->onMessage(msg);
		} catch (const Exception &e) {
			PLN(L"Unhandled exception in window thread: " << e);
			if (app->appWait)
				app->appWait->terminate();
			return noResult();
		}
	}

	LRESULT App::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
		wc.hInstance = hInstance;
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

	void App::initCom() {
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);
	}

	App *app(EnginePtr ptr) {
		LibData *d = ptr.v.data();
		if (!d->app)
			d->app = CREATE(App, ptr.v);
		return d->app.ret();
	}


	/**
	 * Threading logic
	 */

	AppWait::AppWait(Engine &e) : uThread(os::UThread::invalid) {
		app = stormgui::app(e);
		app->release(); // We do not want to hold a ref here.
		app->appWait = this;
		done = false;
	}

	void AppWait::init() {
		threadId = GetCurrentThreadId();
		signalSent = 0;
		currentApp = app;
		uThread = os::UThread::current();

		// Make sure we get a message queue.
		if (!IsGUIThread(TRUE)) {
			assert(false, L"Could not convert to a GUI thread.");
		}
	}

	bool AppWait::wait() {
		if (done) {
			// No more need for message processing!
			return false;
		}

		WaitMessage();
		atomicWrite(signalSent, 0);

		return !done;
	}

	void AppWait::signal() {
		// Only the first thread posts the message if needed.
		if (atomicCAS(signalSent, 0, 1) == 0)
			PostThreadMessage(threadId, WM_THREAD_SIGNAL, 0, 0);
	}

	void AppWait::work() {
		try {
			if (!app->processMessages()) {
				uThread = os::UThread::invalid;
				done = true;
			}
		} catch (const Exception &e) {
			PLN(L"Unhandled exception in window thread: " << e);
			terminate();
		}
	}

	void AppWait::terminate() {
		PostThreadMessage(threadId, WM_QUIT, 0, 0);
	}

}
