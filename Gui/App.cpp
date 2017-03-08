#include "stdafx.h"
#include "App.h"
#include "LibData.h"
#include "Window.h"

namespace gui {

	App::App() : appWait(null), creating(null) {
		windows = new (this) Map<Handle, Window *>();
		liveWindows = new (this) Set<Window *>();
		hInstance = GetModuleHandle(NULL);
		initCommonControls();
		hWindowClass = registerWindowClass();

		TODO(L"Create default font!");
	}

	void App::terminate() {
		TODO(L"Terminate everything!");
		// appWait->terminate();
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

	void App::waitForEvent(Window *owner, Event *event) {
		TODO(L"Implement me!");
		// Is this needed if we use Storm's events?
	}

	MsgResult App::handleMessage(HWND hwnd, const Message &msg) {
		TODO(L"Implement me!");
		return noResult();
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

}
