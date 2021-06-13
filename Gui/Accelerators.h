#pragma once
#include "KeyChord.h"
#include "Handle.h"
#include "Core/Map.h"
#include "Core/Fn.h"

namespace gui {

	class Frame;

	/**
	 * Table of accelerators for a particular window.
	 *
	 * An instance of this table is created by each Frame and associated with the frame itself. When
	 * adding a menu, this table is automatically populated with entries from the menu.
	 */
	class Accelerators : public ObjectOn<Ui> {
		STORM_CLASS;
	public:
		// Create an accelerator table for a particular window.
		STORM_CTOR Accelerators();

		// Destroy.
		virtual ~Accelerators();

		// Add a mapping.
		void STORM_FN add(KeyChord chord, Fn<void> *call);

		// Special version used for menus.
		void add(KeyChord chord, Fn<void> *call, Handle item);

		// Remove a mapping.
		void STORM_FN remove(KeyChord chord);

		// Dispatch a keypress. Returns "true" if the chord was found and dispatched.
		Bool STORM_FN dispatch(KeyChord chord);

		// Attach this accelerator list to a frame. Only relevant on Gtk.
		void attach(Handle to);

	private:
		// A single accelerator (we can't store functions right in the map in C++).
		class Accel {
			STORM_VALUE;
		public:
			Accel() : call(null) {}
			Accel(Fn<void> *call) : call(call) {}

			Fn<void> *call;

			Bool any() const {
				return call != null;
			}
		};

		// Map of all accelerators.
		Map<KeyChord, Accel> *data;

		// On Linux, we also maintain an AccelGroup and let Gtk+ handle the accelerators
		// as far as possible.
		UNKNOWN(PTR_NOGC) void *gtkData;

		// On Linux, we need a nonmoving object to pass as a parameter to the callback.
		UNKNOWN(PTR_GC) void *staticData;
	};

}
