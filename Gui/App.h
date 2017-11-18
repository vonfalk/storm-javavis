#pragma once
#include "Core/EnginePtr.h"
#include "Core/TObject.h"
#include "Core/Map.h"
#include "Core/Set.h"
#include "Core/Event.h"
#include "Handle.h"
#include "Message.h"

namespace gui {

	class Window;
	class Frame;
	class Font;
	class AppWait;

	/**
	 * Application object. One instance of this object is created to manage all live windows for one
	 * Engine. This class is therefore a singleton, get the one and only instance using 'app' below.
	 */
	class App : public ObjectOn<Ui> {
		STORM_CLASS;
	public:
		// Default font.
		Font *defaultFont;

		// Indicate that 'w' is about to create a window and attach it. This makes us forward any
		// messages addressed to an unknown window to 'w'. This is only done until 'addWindow(w)' is
		// called.
		void preCreate(Window *w);

		// Abort creation on failure.
		void createAborted(Window *w);

		// Add a new window.
		void addWindow(Window *w);

		// Remove a window.
		void removeWindow(Window *w);

		// Terminate this app. Terminates the message loop and closes any open windows.
		void terminate();

		// Wait for an event, making sure that we're not blocking the message loop. Also returns
		// when 'window' is no longer alive.
		void waitForEvent(Window *owner, Event *event);

		// Find a registered window.
		MAYBE(Window *) findWindow(Handle h);

		/**
		 * Win32 specifics.
		 */

#ifdef GUI_WIN32
		// Get the window class for frame windows.
		ATOM windowClass();

		// Get our instance.
		HINSTANCE instance();
#endif

	private:
		friend class AppWait;
		friend App *app(EnginePtr e);

		// Create the instance.
		App();

		// # of messages to process before doing a thread switch.
		static const Nat maxProcessMessages = 10;

		// Keep track of all windows.
		typedef Map<Handle, Window *> WindowMap;
		Map<Handle, Window *> *windows;

		// Keep track of all known windows.
		typedef Set<Window *> WindowSet;
		Set<Window *> *liveWindows;

		// The window class that is currently trying to create a window (if any).
		Window *creating;

		// Our window class (an ATOM).
		Handle hWindowClass;

		// Our instance (an HINSTANCE).
		Handle hInstance;

		// The AppWait object that is in charge of our thread.
		UNKNOWN(PTR_NOGC) AppWait *appWait;

		// Platform-specific initialization.
		void init();

		// Check if either the event is set or the window is removed.
		bool resumeEvent(Window *w, Event *e);

		/**
		 * Win32 specific functionality.
		 * Note: No variables may be placed inside #ifdefs, since that confuses the Storm preprocessor.
		 */
#ifdef GUI_WIN32

		// Process any messages for this thread. Returns 'false' if we should quit the message loop for any reason.
		bool processMessages();

		// Process a single message. It is assumed to not be WM_QUIT.
		void processMessage(MSG &msg);

		// Find the root frame for a window.
		Frame *findRoot(HWND wnd);

		// Register our window class.
		ATOM registerWindowClass();

		// Set up other needed things.
		void initCommonControls();

		// Window proc.
		static LRESULT WINAPI windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static MsgResult handleMessage(HWND hwnd, const Message &msg);
#endif
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
		~AppWait();

		virtual void init();
		virtual bool wait(os::IOHandle &io);
		virtual bool wait(os::IOHandle &io, nat msTimeout);
		virtual void signal();
		virtual void work();

		// Ask us to terminate. Notify the semaphore when ready.
		void terminate(os::Sema &notify);

		// Notify that we are currently inside a message handler and should not consider new window
		// messages for the time being. (recursive, calling 'disable' twice and 'enable' once leaves
		// it disables).  This is done since the Win32 api gets very confused if we pre-empt a
		// thread with Win32 api calls on the stack. This happens when another UThread called some
		// Win32-function that invokes a callback to the Window proc, which in turn needs to wait
		// for something (eg. redraws).
		void disableMsg();
		void enableMsg();

		// Are we done with our duty?
		bool isDone() const { return done; }

		// The UThread that runs the message pump.
		os::UThread uThread;

	private:
		// # of WM_THREAD_SIGNAL messages sent. Not actually increased above 1
		nat signalSent;

		// Message checking disabled?
		nat msgDisabled;

		// Condition for use when messages are disabled.
		os::IOCondition fallback;

		// Engine, used to retrieve the App object.
		Engine &e;

		// Should we exit?
		bool done;

		// Notify this semaphore on exit.
		os::Sema *notifyExit;

		// Destroy.
		void destroy();

#ifdef GUI_WIN32
		// Our thread id (win32 thread id).
		DWORD threadId;
#endif

#ifdef GUI_GTK
		// The global main context.
		GMainContext *context;

		// The source we use to attach file descriptors to.
		GSource *fdSource;

		// Data for our fdSource.
		struct FdData {
			GSource super;
			GMainContext *context;
			os::IOHandle::Desc *fds;
			gpointer *tags;
			nat *notify;
		};

		// Function table for our GSource.
		static GSourceFuncs fdFuncs;

		// Callback from fdFuncs.
		static gboolean fdCheck(GSource *source);

		// Callback from GLib.
		static gboolean onTimeout(gpointer appWait);

		// Used in 'fdCheck'.
		vector<gpointer> tags;

		// Wait for a callback of some sort.
		void doWait(os::IOHandle &io);

#endif

	};

	os::Thread spawnUiThread(Engine &e);
}
