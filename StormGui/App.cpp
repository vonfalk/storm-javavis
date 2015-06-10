#include "stdafx.h"
#include "App.h"
#include "Window.h"
#include "Frame.h"

namespace stormgui {

	App::App() : appWait(null) {
		windowClass = registerWindowClass();
	}

	App::~App() {
		TODO(L"Reap any live windows!");
	}

	Frame *App::findRoot(HWND wnd) {
		HWND parent = GetParent(wnd);
		WindowMap::iterator i = windows.find(parent);
		if (i == windows.end()) {
			assert(false, L"Parent not found!");
			return null;
		}

		return i->second->rootFrame();
	}

	Frame *App::addWindow(Window *w) {
		HWND handle = w->handle();

		windows.insert(make_pair(handle, w));

		if (Frame *f = as<Frame>(w)) {
			return f;
		} else {
			return findRoot(handle);
		}
	}

	void App::removeWindow(Window *w) {
		windows.erase(w->handle());
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
		if (i == windows.end()) {
			WARNING(L"Unknown window: " << msg.hwnd << L", ignoring message.");
			return;
		}

		Window *w = i->second;

		// Dialog message?
		Frame *root = w->rootFrame();
		if (root && IsDialogMessage(root->handle(), &msg))
			return;

		// Translate.
		TranslateMessage(&msg);

		// Now, we can handle it! Since it is a mess to use the window-proc parameter in the
		// window class and passing extra parameters, we intercept the message before we do that
		// and process it if needed. Else, we send it on to the window-proc (which is the default
		// window proc in our case), to handle it. This actually allows us to override messages in
		// common controls if we want to, and maybe we do not need the preTranslateMessage!
		// TODO: This approach will _not_ work well when we're handling messages sent from this thread
		// sent using SendMessage (possibly from Common Controls).
		MsgResult r = w->onMessage(msg);
		if (!r.any) {
			// Unhandled message, leave it to the default window proc.
			DispatchMessage(&msg);
		} else {
			if (InSendMessage())
				ReplyMessage(r.result);
		}
	}

	ATOM App::registerWindowClass() {
		static ATOM c = 0;

		// Someone already created our window class! (maybe this is bad due to different icons?)
		if (c)
			return c;

		// TODO: Is this ok in our case? This returns the HINSTANCE of the exe-file, but that is
		// probably what we want in this case.
		HINSTANCE instance = GetModuleHandle(NULL);

		WNDCLASSEX wc;
		zeroMem(wc);
		wc.cbSize = sizeof(WNDCLASSEX);
		TODO(L"Load icon from somewhere!");
		// HICON icon = LoadIcon(HINSTANCE, MAKEINTRESOURCE(iconResource));
		HICON icon = LoadIcon(NULL, IDI_APPLICATION);
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = &DefWindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = instance;
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

	App *app(EnginePtr ptr) {
		LibData *d = ptr.v.data();
		if (!d->app)
			d->app = CREATE(App, ptr.v);
		return d->app.ret();
	}


	/**
	 * Threading logic
	 */

	AppWait::AppWait(Engine &e) {
		app = stormgui::app(e);
		app->release(); // We do not want to hold a ref here.
		app->appWait = this;
		done = false;
	}

	void AppWait::init() {
		threadId = GetCurrentThreadId();
		signalSent = 0;

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
		if (!app->processMessages())
			done = true;
	}

	void AppWait::terminate() {
		PostThreadMessage(threadId, WM_QUIT, 0, 0);
	}

}
