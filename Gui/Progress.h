#pragma once
#include "Window.h"

namespace gui {

	/**
	 * Progress bar.
	 *
	 * Note: The text of the window is not shown anywhere.
	 */
	class Progress : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR Progress();

		virtual Size STORM_FN minSize();

		// Get the progress.
		Float STORM_FN progress() { return myProgress; }

		// Set the progress. Also make sure the progress bar is not in wait mode.
		void STORM_ASSIGN progress(Float v);

		// Seet the progress into wait mode. This is until the progress is set again.
		void STORM_FN wait();

	protected:
		virtual bool create(ContainerBase *parent, nat id);

		virtual void windowDestroyed();

	private:
		// Current progress.
		Float myProgress;

		// Are we in wait mode.
		Bool waitMode;

		// Timer ID in Gtk+.
		Nat timerId;
	};

}
