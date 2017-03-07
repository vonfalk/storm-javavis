#pragma once
#include "Core/EnginePtr.h"
#include "Core/TObject.h"
#include "Core/Map.h"
#include "Core/Set.h"
#include "Handle.h"
#include "Message.h"

namespace gui {

	class Window;
	class Frame;

	/**
	 * Application object. One instance of this object is created to manage all live windows for one
	 * Engine. This class is therefore a singleton, get the one and only instance using 'app' below.
	 */
	class App : public ObjectOn<Ui> {
		STORM_CLASS;
	public:
		// Indicate that 'w' is about to create a window and attach it. This makes us forward any
		// messages addressed to an unknown window to 'w'. This is only done until 'addWindow(w)' is
		// called.
		void preCreate(Window *w);

		// Abort creation on failure.
		void createAborted(Window *w);

		// Remove a window.
		void removeWindow(Window *w);

		// Terminate this app. Terminates the message loop and closes any open windows.
		void terminate();

		// Wait for an event, making sure that we're not blocking the message loop. Also returns
		// when 'window' is no longer alive.
		void waitForEvent(Window *owner, os::Event &event);

		// Default font.
		// Font *defaultFont;

		// Get our instance.
		HINSTANCE instance();

	private:
		friend class AppWait;
		friend App *app(EnginePtr e);

		// Create the instance.
		App();

		// # of messages to process before doing a thread switch.
		static const Nat maxProcessMessages = 10;

		// Process any messages for this thread. Returns 'false' if we should quit the message loop for any reason.
		bool processMessages();

		// Process a single message. It is assumed to not be WM_QUIT.
		void processMessage(MSG &msg);

		// Check if either the event is set or the window is removed.
		bool resumeEvent(Window *w, os::Event &e);

		// Keep track of all windows.
		typedef Map<Handle, Window *> WindowMap;
		Map<Handle, Window *> *windows;

		// Keep track of all known windows.
		typedef Set<Window *> WindowSet;
		Set<Window *> *liveWindows;

		// The window class that is currently trying to create a window (if any).
		Window *creating;

		// Find the root frame for a window.
		Frame *findRoot(HWND wnd);

		// Register our window class.
		ATOM registerWindowClass();

		// Set up other needed things.
		void initCommonControls();

		// Our window class (an ATOM).
		Handle hWindowClass;

		// Our instance (an HINSTANCE).
		Handle hInstance;

		// The AppWait object that is in charge of our thread.
		UNKNOWN(PTR_NOGC) AppWait *appWait;

		// Window proc.
		static LRESULT WINAPI windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static MsgResult handleMessage(HWND hwnd, const Message &msg);
	};

	// Get the App object.
	App *STORM_FN app(EnginePtr engine) ON(Ui);

	/**
	 * Custom implementation of the thread waiting code. Uses the window main loop as a wait, sends
	 * a WM_THREAD_SIGNAL (defined in stdafx.h) message to wake up the waiting thread. This allows
	 * us to run the Windows message pump as a part of our threading runtime.
	 */
	class AppWait : public os::ThreadWait {
	public:
		AppWait(Engine &e);

		// TODO!
	};

}
