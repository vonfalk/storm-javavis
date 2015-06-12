#pragma once
#include "StormGui.h"
#include "Message.h"

namespace stormgui {

	class Window;
	class Frame;
	class AppWait;

	/**
	 * Application object. One instance of this object is created to manage all live windows for one
	 * Engine. This class is therefore a singleton class, get the one and only instance (for the
	 * Engine) using the app() function below.
	 */
	class App : public ObjectOn<Ui> {
		STORM_CLASS;
	public:
		// Destroy.
		~App();

		// Indicate that 'w' is about to create a window and attach it. This makes us forward any
		// messages to an unknown window to 'w'. This is only done until addWindow('w') is done.
		void preCreate(Window *w);

		// Abort creation on failure.
		void createAborted(Window *w);

		// Add a new window.
		void addWindow(Window *w);

		// Remove a window.
		void removeWindow(Window *w);

		// Terminate this app. Terminates the message loop. TODO? Reap windows early?
		void terminate();

		// Wait for an event, making sure that we're not blocking the message loop. Also returns when
		// 'window' is no longer alive.
		void waitForEvent(Window *owner, os::Event &event);

		// Get the window class for frame windows.
		ATOM windowClass();

		// Get our instance.
		HINSTANCE instance();

	private:
		friend class AppWait;
		friend App *app(EnginePtr);

		// # of messages to process before doing a thread switch.
		static const nat maxProcessMessages = 10;

		// Create the instance.
		App();

		// Process any messages for this thread. Returns 'false' if we should quit the message loop for any reason.
		bool processMessages();

		// Process a single message. It is assumed to not be WM_QUIT.
		void processMessage(MSG &msg);

		// Check if either the event is set or the window is removed.
		bool resumeEvent(Window *w, os::Event &e);

		// Keep track of all windows. These are borrowed pointers, but we get notified about them being destroyed.
		typedef hash_map<HWND, Window *> WindowMap;
		WindowMap windows;

		// Keep track of all known windows.
		typedef hash_set<Window *> WindowSet;
		WindowSet liveWindows;

		// The window class that is currently trying to create a window (if any).
		Window *creating;

		// Find the root frame for a window.
		Frame *findRoot(HWND wnd);

		// Register our window class.
		ATOM registerWindowClass();

		// Set up other needed things.
		void initCommonControls();
		void initCom();

		// Our window class.
		ATOM hWindowClass;

		// Our instance.
		HINSTANCE hInstance;

		// The AppWait object that is in charge of our thread.
		AppWait *appWait;

		// Window proc.
		static LRESULT WINAPI windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static MsgResult handleMessage(HWND hwnd, const Message &msg);
	};


	// Get the instance of App.
	App *STORM_ENGINE_FN app(EnginePtr engine) ON(Ui);


	/**
	 * Custom implementation of the thread waiting code. Uses the window main loop as a wait, sends
	 * a WM_THREAD_SIGNAL (defined in stdafx.h) message to wake up the waiting thread. This allows
	 * us to run the Windows message pump as a part of our threading runtime.
	 */
	class AppWait : public os::ThreadWait {
	public:
		AppWait(Engine &e);

		virtual void init();
		virtual bool wait();
		virtual void signal();
		virtual void work();

		// Ask us to terminate.
		void terminate();

		// The UThread that runs the message pump.
		os::UThread uThread;

	private:
		// Our thread id (win32 thread id).
		DWORD threadId;

		// # of WM_THREAD_SIGNAL messages sent. Not actually increased above 1
		nat signalSent;

		// This thread's App object. Not owned since the DllData keeps it alive.
		App *app;

		// Should we exit?
		bool done;
	};


}
