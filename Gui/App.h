#pragma once
#include "Core/EnginePtr.h"
#include "Core/TObject.h"

namespace gui {

	/**
	 * Application object. One instance of this object is created to manage all live windows for one
	 * Engine. This class is therefore a singleton, get the one and only instance using 'app' below.
	 */
	class App : public ObjectOn<Ui> {
		STORM_CLASS;
	public:
		// Destroy all windows and prepare for termination.
		void destroy();

	private:
		friend class AppWait;
		friend App *app(EnginePtr e);

		// Create the instance.
		App();
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
