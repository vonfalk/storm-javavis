#pragma once
#include "StormGui.h"

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

		// Add a new window, returns the root frame.
		Frame *addWindow(Window *w);

		// Remove a window.
		void removeWindow(Window *w);

		// Terminate this app. Terminates the message loop. TODO? Reap windows early?
		void terminate();

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

		// Keep track of all windows. These are borrowed pointers, but we get notified about them being destroyed.
		typedef hash_map<HWND, Window *> WindowMap;
		WindowMap windows;

		// Find the root frame for a window.
		Frame *findRoot(HWND wnd);

		// Register our window class.
		ATOM registerWindowClass();

		// Our window class.
		ATOM windowClass;

		// The AppWait object that is in charge of our thread.
		AppWait *appWait;
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
