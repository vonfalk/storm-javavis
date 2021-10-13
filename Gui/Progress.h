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

		// Get the position.
		Nat STORM_FN value() { return myValue; }
		Nat STORM_FN v() { return myValue; }

		// Set the position.
		void STORM_ASSIGN value(Nat v);
		void STORM_ASSIGN v(Nat v);

		// Get the maximum position.
		Nat STORM_FN max() { return myMax; }

		// Set the maximum position.
		void STORM_ASSIGN max(Nat v);

		// Get "unknown" progres (i.e. we don't know how long is left).
		Bool STORM_FN unknown() { return myUnknown; }

		// Set "unknown" progres (i.e. we don't know how long is left).
		void STORM_ASSIGN unknown(Bool enable);

	protected:
		virtual bool create(ContainerBase *parent, nat id);

	private:
		// Current value.
		Nat myValue;

		// Maximum position.
		Nat myMax;

		// Unknown progress.
		Bool myUnknown;
	};

}
