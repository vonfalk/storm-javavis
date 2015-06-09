#pragma once
#include "StormGui.h"

namespace stormgui {

	class Window;
	class Frame;

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

	private:
		friend App *app(EnginePtr);

		// Create the instance.
		App();

		// Main message loop to be executed on the Ui thread.
		void messageLoop();

		// Keep track of all windows. These are borrowed pointers, but we get notified about them being destroyed.
		typedef hash_map<HWND, Window *> WindowMap;
		WindowMap windows;

		// Find the root frame for a window.
		Frame *findRoot(HWND wnd);

		// Register our window class.
		ATOM registerWindowClass();

		// Our window class.
		ATOM windowClass;
	};


	// Get the instance of App.
	App *STORM_ENGINE_FN app(EnginePtr engine) ON(Ui);

}
