#include "stdafx.h"
#include "ScrollWindow.h"
#include "Container.h"
#include "Exception.h"

namespace gui {

	ScrollWindow::ScrollWindow(Window *inside) : child(inside), hScroll(false), vScroll(true) {
		if (inside->parent())
			throw new (this) GuiError(S("Can not attach a child multiple times or to multiple parents."));
		child->attachParent(this);
	}

	ScrollWindow::ScrollWindow(Window *inside, Size minSize)
		: child(inside), minSz(minSize), hScroll(false), vScroll(true) {

		if (inside->parent())
			throw new (this) GuiError(S("Can not attach a child multiple times or to multiple parents."));
		child->attachParent(this);
	}

	void ScrollWindow::minSize(Size sz) {
		minSz = sz;
	}

	Size ScrollWindow::minSize() {
		Size ch = child->minSize();

		if (hScroll) {
			ch.w = min(ch.w, minSz.w);
		} else {
			ch.w = max(ch.w, minSz.w);
		}

		if (vScroll) {
			ch.h = min(ch.h, minSz.h);
		} else {
			ch.h = max(ch.h, minSz.h);
		}

		return ch;
	}

	void ScrollWindow::parentCreated(nat id) {
		Window::parentCreated(id);

		// Create the child with id 1.
		child->parentCreated(1);
	}

	void ScrollWindow::windowDestroyed() {
		Window::windowDestroyed();

		if (child->created())
			child->handle(invalid);
	}

	void ScrollWindow::resized(Size size) {
		Window::resized(size);

		Size sz = child->minSize();
		Rect original = child->pos();
		if (!hScroll) {
			sz.w = size.w;
		}

		if (!vScroll) {
			sz.h = size.h;
		}

		child->pos(Rect(original.p0, sz));
	}


#ifdef GUI_WIN32

	bool ScrollWindow::create(ContainerBase *parent, nat id) {
		DWORD myFlags = 0;
		if (hScroll)
			myFlags |= WS_HSCROLL;
		if (vScroll)
			myFlags |= WS_VSCROLL;
		return createEx(NULL, childFlags | myFlags, 0, parent->handle().hwnd(), id);
	}

	void ScrollWindow::horizontal(Bool v) {
		hScroll = v;

		if (created()) {
			ShowScrollBar(handle().hwnd(), SB_HORZ, hScroll ? TRUE : FALSE);
		}
	}

	void ScrollWindow::vertical(Bool v) {
		vScroll = v;

		if (created()) {
			ShowScrollBar(handle().hwnd(), SB_VERT, vScroll ? TRUE : FALSE);
		}
	}

#endif

}
