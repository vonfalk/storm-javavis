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

		// Compensate for the size of the actual scrollbars.
		if (hScroll)
			ch.h += GetSystemMetrics(SM_CYHSCROLL);
		if (vScroll)
			ch.w += GetSystemMetrics(SM_CXVSCROLL);

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
		if (!hScroll) {
			sz.w = size.w;
		}

		if (!vScroll) {
			sz.h = size.h;
		}

		setChildSize(sz);
		updateBars(sz);
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

	MsgResult ScrollWindow::onMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_HSCROLL:
			return msgResult(onScroll(msg.wParam, SB_HORZ));
		case WM_VSCROLL:
			return msgResult(onScroll(msg.wParam, SB_VERT));
		case WM_MOUSEWHEEL:
			return msgResult(onWheel(GET_WHEEL_DELTA_WPARAM(msg.wParam), SB_VERT));
		case WM_MOUSEHWHEEL:
			return msgResult(onWheel(GET_WHEEL_DELTA_WPARAM(msg.wParam), SB_HORZ));
		}

		return Window::onMessage(msg);
	}

	LRESULT ScrollWindow::onScroll(WPARAM param, int which) {
		SCROLLINFO info;
		info.cbSize = sizeof(info);
		info.fMask = SIF_PAGE | SIF_POS | SIF_TRACKPOS | SIF_RANGE;
		GetScrollInfo(handle().hwnd(), which, &info);

		const int lineSize = int(font()->pxHeight());

		switch (LOWORD(param)) {
		case SB_TOP:
			info.nPos = info.nMin;
			break;
		case SB_BOTTOM:
			info.nPos = info.nMax;
			break;
		case SB_LINEUP:
			info.nPos = max(info.nPos - lineSize, info.nMin);
			break;
		case SB_LINEDOWN:
			info.nPos = min(info.nPos + lineSize, info.nMax - int(info.nPage));
			break;
		case SB_PAGEDOWN:
			info.nPos = max(info.nPos - int(info.nPage), info.nMin);
			break;
		case SB_PAGEUP:
			info.nPos = min(info.nPos + int(info.nPage), info.nMax - int(info.nPage));
			break;
		case SB_THUMBTRACK:
			info.nPos = info.nTrackPos;
			break;
		default:
			return 0;
		}

		info.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
		SetScrollInfo(handle().hwnd(), which, &info, TRUE);

		int x = info.nPos, y = info.nPos;
		if (which == SB_HORZ) {
			SCROLLINFO info;
			info.cbSize = sizeof(info);
			info.fMask = SIF_POS;
			GetScrollInfo(handle().hwnd(), SB_VERT, &info);
			y = info.nPos;
		} else {
			SCROLLINFO info;
			info.cbSize = sizeof(info);
			info.fMask = SIF_POS;
			GetScrollInfo(handle().hwnd(), SB_HORZ, &info);
			x = info.nPos;
		}
		SetWindowPos(child->handle().hwnd(), NULL, -x, -y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);

		return 0;
	}

	LRESULT ScrollWindow::onWheel(int delta, int which) {
		SCROLLINFO info;
		info.cbSize = sizeof(info);
		info.fMask = SIF_PAGE | SIF_POS | SIF_TRACKPOS | SIF_RANGE;
		GetScrollInfo(handle().hwnd(), which, &info);

		const int lineSize = int(font()->pxHeight());

		info.nPos = info.nPos - delta / 2;
		if (info.nPos < info.nMin)
			info.nPos = info.nMin;
		if (info.nPos > info.nMax - int(info.nPage))
			info.nPos = info.nMax - int(info.nPage);

		info.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
		SetScrollInfo(handle().hwnd(), which, &info, TRUE);

		int x = info.nPos, y = info.nPos;
		if (which == SB_HORZ) {
			SCROLLINFO info;
			info.cbSize = sizeof(info);
			info.fMask = SIF_POS;
			GetScrollInfo(handle().hwnd(), SB_VERT, &info);
			y = info.nPos;
		} else {
			SCROLLINFO info;
			info.cbSize = sizeof(info);
			info.fMask = SIF_POS;
			GetScrollInfo(handle().hwnd(), SB_HORZ, &info);
			x = info.nPos;
		}
		SetWindowPos(child->handle().hwnd(), NULL, -x, -y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);

		return 0;
	}

	void ScrollWindow::setScrollInfo(int which, Float childSz, Float ourSz) {
		SCROLLINFO info;
		info.cbSize = sizeof(info);
		info.fMask = SIF_POS;
		GetScrollInfo(handle().hwnd(), which, &info);

		info.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		info.nMin = 0;
		info.nMax = int(childSz) - 1;
		info.nPage = int(ourSz);
		info.nPos = min(info.nPos, info.nMax - int(info.nPage));
		SetScrollInfo(handle().hwnd(), which, &info, TRUE);
	}

	void ScrollWindow::updateBars(Size sz) {
		RECT ourSize;
		GetClientRect(handle().hwnd(), &ourSize);

		if (hScroll)
			setScrollInfo(SB_HORZ, sz.w, Float(ourSize.right));
		if (vScroll)
			setScrollInfo(SB_VERT, sz.h, Float(ourSize.bottom));

		// Update the position of the child, since we might need less scrolling.
		SCROLLINFO info;
		info.cbSize = sizeof(info);
		info.fMask = SIF_POS;
		GetScrollInfo(handle().hwnd(), SB_VERT, &info);
		int y = info.nPos;
		GetScrollInfo(handle().hwnd(), SB_HORZ, &info);
		int x = info.nPos;
		SetWindowPos(child->handle().hwnd(), NULL, -x, -y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	}

	void ScrollWindow::setChildSize(Size sz) {
		if (!child->created())
			return;

		SetWindowPos(child->handle().hwnd(), NULL, 0, 0, (int)sz.w, (int)sz.h, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	}

#endif

}
