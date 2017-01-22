#include "stdafx.h"
#include "Container.h"
#include "Exception.h"
#include <limits>

namespace stormgui {

	// Reserved ids.
	static const nat reservedTo = IDCANCEL;

	Container::Container() {}

	void Container::add(Par<Window> child) {
		if (steal(child->parent()))
			throw GuiError(L"Can not attach a child multiple times or to multiple parents.");

		nat id = allocate(child);
		child->attachParent(this);

		if (created())
			child->parentCreated(id);
	}

	void Container::remove(Par<Window> child) {
		WinMap::iterator i = windows.find(child.borrow());
		if (i == windows.end())
			return;

		child->detachParent();
		ids.erase(i->second);
		windows.erase(i);
	}

	void Container::parentCreated(nat id) {
		Window::parentCreated(id);

		// Late creation, create any remaining children!
		for (IdMap::iterator i = ids.begin(), end = ids.end(); i != end; ++i) {
			Window *w = i->second.borrow();
			if (!w->created())
				w->parentCreated(i->first);
		}
	}

	void Container::windowDestroyed() {
		Window::windowDestroyed();

		for (IdMap::iterator i = ids.begin(), end = ids.end(); i != end; ++i) {
			Window *w = i->second.borrow();
			if (w->created())
				w->handle(invalid);
		}
	}

	nat Container::allocate(Par<Window> window) {
		nat lastUsed = reservedTo;
		if (ids.size() > 0)
			// Take one more than the highest currently used id.
			lastUsed = ids.rbegin()->first;

		nat firstFree;
		// Fallback in rare cases with a lot of creation/destruction.
		if (lastUsed == std::numeric_limits<nat>::max()) {
			WARNING(L"Wrapping. Tab order will not be as expected!");
			// Wrap around and find something not used... We will most likely not have 2^32 controls
			// in a window, so we should find an empty id somewhere!
			for (firstFree = reservedTo + 1; ids.count(firstFree) == 1; firstFree++)
				;
		} else {
			firstFree = lastUsed + 1;
		}

		ids.insert(make_pair(firstFree, window));
		windows.insert(make_pair(window.borrow(), firstFree));
		return firstFree;
	}

	void Container::allocate(Par<Window> window, nat id) {
		if (id == 0)
			return;

		if (ids.count(id) == 1)
			throw GuiError(L"The id " + ::toS(id) + L" is already in use.");

		ids.insert(make_pair(id, window));
		windows.insert(make_pair(window.borrow(), id));
	}

	MsgResult Container::onMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_COMMAND:
			if (onCommand(msg))
				return msgResult(0);
			break;
		}

		return Window::onMessage(msg);
	}

	bool Container::onCommand(const Message &msg) {
		nat type = HIWORD(msg.wParam);
		nat id = LOWORD(msg.wParam);

		// Menus and accelerators. Not implemented yet.
		if (msg.lParam == 0)
			return false;

		IdMap::iterator i = ids.find(id);
		if (i == ids.end())
			return false;

		return i->second->onCommand(type);
	}

}
