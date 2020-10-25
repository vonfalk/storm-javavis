#pragma once
#include "Core/EnginePtr.h"
#include "Core/TObject.h"
#include "Core/Map.h"
#include "Core/Set.h"
#include "Core/Event.h"
#include "Handle.h"
#include "Message.h"
#include "Menu.h"

namespace gui {

	class Window;
	class Frame;
	class Font;
	class AppWait;

	// Get the default font.
	Font *STORM_FN defaultFont(EnginePtr e) ON(Ui);

	// Get the default background color.
	Color STORM_FN defaultBgColor(EnginePtr e) ON(Ui);

	/**
	 * Application object. One instance of this object is created to manage all live windows for one
	 * Engine. This class is therefore a singleton, get the one and only instance using 'app' below.
	 */
	class App : public ObjectOn<Ui> {
		STORM_CLASS;
	public:
		// Default font.
		Font *defaultFont;

		// Default background color.
		Color defaultBgColor;

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

		// Find a menu item from its handle. This searches all menus attached to all top-level
		// windows, so don't use it for frequent events.
		MAYBE(Menu::Item *) findMenuItem(Handle h);

#ifdef GUI_WIN32
		/**
		 * Win32 specifics.
		 */

		// Get the window class for frame windows.
		ATOM windowClass();

		// Get our instance.
		HINSTANCE instance();

		// Called before/after modal dialogs are shown, to indicate that we will enter a second window loop.
		void showDialog(bool show);
#endif
#ifdef GUI_GTK
		// Get the default display.
		inline GdkDisplay *defaultDisplay() const { return display; }
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

		// The default display when using Gtk+.
		GdkDisplay *display;

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
		virtual void setup();
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

#ifdef GUI_WIN32
		// Monitor messages to figure out when (and if) we need to enable a timer to keep the
		// threading system responsive. Some operations enter a recursive window loop, which
		// bypasses our efforts to keep other UThreads alive etc.
		// We can't do this ourselves, as we will only see messages directly from PeekMessage.
		// But the required timer messages are posted directly to the desired window by the
		// recursive window loop, making it impossible for us to get hold of it otherwise.
		// 'false' means that this message should not be processed any further.
		bool checkBlockMsg(HWND hWnd, const Message &msg);

		// Notify before and after some part of the code attempts to create a dialog, so that we can
		// keep the other UThreads interactive.
		void showDialog(bool show);
#endif
#ifdef GUI_GTK
		// Post a repaint request to the Gtk+ window. Safe to call from any thread.
		void repaint(Handle handle);
#endif

	private:
		// # of WM_THREAD_SIGNAL messages sent. Not actually increased above 1
		nat signalSent;

		// Message checking disabled?
		nat msgDisabled;

		// Engine, used to retrieve the App object.
		Engine &e;

		// Should we exit?
		bool done;

		// Notify this semaphore on exit.
		os::Sema *notifyExit;

		// Platform specific init and destroy.
		void platformInit();
		void platformDestroy();

#ifdef GUI_WIN32
		// Condition for use when messages are disabled.
		os::IOCondition fallback;

		// Our thread id (win32 thread id).
		DWORD threadId;

		// Keep track of what each window is doing regarding potentially blocking the main message loop.
		enum BlockStatus {
			// When the user has actually started moving the window.
			blockMoving,

			// When a menu is showing.
			blockMenu,

			// Modal dialog is showing. This is used for the NULL value, as this is not necessarily
			// associated with a window.
			blockModal,
		};

		// Each window we want to keep track of. Windows that are not affecting the block status
		// should not be present in the map, thus it is easy to determine if we need to enable the
		// timer. Just check if there is anything in here.
		typedef map<HWND, BlockStatus> BlockMap;
		BlockMap blockStatus;

		// Number of modal dialogs showing that are out of our control.
		Nat blockingDialogs;

		// Is the blocking timer active? If so, what window is it attached to?
		HWND blockTimer;
#endif

#ifdef GUI_GTK
		// The global main context.
		GMainContext *context;

		// File descriptor for the eventfd we use as a condition.
		int eventFd;

		// Did we call 'wait' since the last call to 'dispatch'?
		bool dispatchReady;

		// Poll descriptors passed to Gtk+.
		vector<GPollFD> gPollFd;

		// Poll descriptors used by the system calls.
		vector<struct pollfd> pollFd;

		// Wait for a callback of some sort.
		void doWait(os::IOHandle &io, int timeout);

		// Low-level waiting primitives.
		void gtkWait(os::IOHandle::Desc &io, int timeout);
		void plainWait(struct pollfd *fds, size_t count, int timeout);

		// Hook for Gtk+ events.
		static void gtkEventHook(GdkEvent *event, gpointer data);

#endif

	};

	os::Thread spawnUiThread(Engine &e);
}
