#include "stdafx.h"
#include "Container.h"
#include "Exception.h"
#include <limits>

namespace gui {

#ifdef GUI_WIN32
	// Reserved ids.
	static const Nat reservedTo = IDCANCEL;
#else
	static const Nat reservedTo = 0;
#endif

	Container::Container() {
		ids = new (this) IdMap();
		windows = new (this) WinMap();
		lastId = reservedTo;
	}

	void Container::add(Window *child) {
		if (child->parent())
			throw GuiError(L"Can not attach a child multiple times or to multiple parents.");

		nat id = allocate(child);
		child->attachParent(this);

		if (created())
			child->parentCreated(id);
	}

	void Container::remove(Window *child) {
		if (!windows->has(child))
			return;

		child->detachParent();
		Nat id = windows->get(child);
		ids->remove(id);
		windows->remove(child);
	}

	void Container::parentCreated(Nat id) {
		Window::parentCreated(id);

		// Late creation, create any remaining children!
		for (IdMap::Iter i = ids->begin(), end = ids->end(); i != end; ++i) {
			Window *w = i.v();
			if (!w->created())
				w->parentCreated(i.k());
		}
	}

	void Container::windowDestroyed() {
		Window::windowDestroyed();

		for (IdMap::Iter i = ids->begin(), end = ids->end(); i != end; ++i) {
			Window *w = i.v();
			if (w->created())
				w->handle(invalid);
		}
	}

	Nat Container::allocate(Window *window) {
		Nat firstFree;
		// Fallback in rare cases with a lot of creation/destruction.
		if (lastId == std::numeric_limits<Nat>::max()) {
			WARNING(L"Wrapping. Tab order will not be as expected!");
			// Wrap around and find something not used... We will most likely not have 2^32 controls
			// in a window, so we should find an empty id somewhere!
			for (firstFree = reservedTo + 1; ids->has(firstFree); firstFree++)
				;
		} else {
			firstFree = ++lastId;
		}

		ids->put(firstFree, window);
		windows->put(window, firstFree);
		return firstFree;
	}

	void Container::allocate(Window *window, Nat id) {
		if (id == 0)
			return;

		if (ids->has(id))
			throw GuiError(L"The id " + ::toS(id) + L" is already in use.");

		ids->put(id, window);
		windows->put(window, id);
	}

#ifdef GUI_WIN32

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

		IdMap::Iter i = ids->find(id);
		if (i == ids->end())
			return false;

		return i.v()->onCommand(type);
	}

#endif
}
