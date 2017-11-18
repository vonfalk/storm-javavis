#pragma once
#include "Window.h"
#include "Core/Map.h"

namespace gui {

	/**
	 * A container is a window that can contain child windows.
	 *
	 * Manages IDs of child windows and forwards messages from child windows to their corresponding
	 * classes here.
	 */
	class Container : public Window {
		STORM_CLASS;
	public:
		// Create a container.
		STORM_CTOR Container();

		// Add a child window. This may actually create it!
		void STORM_FN add(Window *child);

		// Remove a child window. Silently fails if the child does not already exist.
		void STORM_FN remove(Window *child);

		// Called when our parent is created.
		virtual void parentCreated(nat id);

		// Called when we've been destroyed.
		virtual void windowDestroyed();

#ifdef GUI_WIN32
		// Handle messages.
		virtual MsgResult onMessage(const Message &msg);

		// Forward WM_COMMAND messages to the right window.
		bool onCommand(const Message &msg);
#endif
	private:
		// Currently used ids.
		typedef Map<Nat, Window *> IdMap;
		Map<Nat, Window *> *ids;

		// Map the other way as well.
		typedef Map<Window *, Nat> WinMap;
		Map<Window *, Nat> *windows;

		// Last used control id.
		Nat lastId;

		// Allocate a new id for the window.
		Nat allocate(Window *window);

		// Allocate a window with a suggested id. Fails if that id is already in use.
		// An id of 0 is special. That means you do not want any callbacks from that control.
		void allocate(Window *window, Nat id);
	};

}
