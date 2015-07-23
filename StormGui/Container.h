#pragma once
#include "Window.h"

namespace stormgui {

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
		void STORM_FN add(Par<Window> child);

		// Remove a child window. Silently fails if the child does not already exist.
		void STORM_FN remove(Par<Window> child);

		// Called when our parent is created.
		virtual void parentCreated(nat id);

		// Called when we're about to be destroyed.
		virtual void windowDestroyed();

		// Handle messages.
		virtual MsgResult onMessage(const Message &msg);

	private:
		// Currently used ids.
		typedef map<nat, Auto<Window>> IdMap;
		IdMap ids;

		// Map the other way as well.
		typedef map<Window *, nat> WinMap;
		WinMap windows;

		// Allocate a new id for the window.
		nat allocate(Par<Window> window);

		// Allocate a window with a suggested id. Fails if that id is already in use.
		// An id of 0 is special. That means you do not want any callbacks from that control.
		void allocate(Par<Window> window, nat id);

		// Forward WM_COMMAND messages to the right window.
		bool onCommand(const Message &msg);
	};

}
