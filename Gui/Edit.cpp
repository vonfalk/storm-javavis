#include "stdafx.h"
#include "Edit.h"

namespace gui {

	Selection::Selection() : start(0), end(0) {}

	Selection::Selection(Nat s) : start(s), end(s) {}

	Selection::Selection(Nat s, Nat e) : start(s), end(e) {}


	Edit::Edit() {
		myMultiline = false;
		onReturn = null;
	}

	Edit::Edit(Str *cue) {
		myMultiline = false;
		myCue = cue;
	}

	bool Edit::create(HWND parent, nat id) {
		DWORD flags = editFlags;
		if (myMultiline)
			flags |= ES_MULTILINE | WS_VSCROLL | ES_WANTRETURN;

		if (!Window::createEx(WC_EDIT, flags, WS_EX_CLIENTEDGE, parent, id))
			return false;

		// Update selection.
		selected(sel);

		// Cue banner.
		SendMessage(handle(), EM_SETCUEBANNER, 0, (LPARAM)myCue->c_str());

		return true;
	}

	Bool Edit::onChar(Nat code) {
		if (code == 127 && pressed(VK_CONTROL)) {
			removeLastWord();
			return true;
		}
		return false;
	}

	Bool Edit::onKey(Bool down, Nat code) {
		if (down && code == VK_RETURN) {
			if (!myMultiline || pressed(VK_CONTROL)) {
				if (onReturn) {
					onReturn->call(this);
					return true;
				}
			}
		}
		return false;
	}

	Selection Edit::selected() {
		if (created()) {
			DWORD start, end;
			SendMessage(handle(), EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
			sel.start = start;
			sel.end = end;
		}
		return sel;
	}

	void Edit::selected(Selection sel) {
		this->sel = sel;
		if (created())
			SendMessage(handle(), EM_SETSEL, (WPARAM)sel.start, (LPARAM)sel.end);
	}

	Str *Edit::cue() {
		return myCue;
	}

	void Edit::cue(Str *s) {
		myCue = s;
		if (created())
			SendMessage(handle(), EM_SETCUEBANNER, 0, (LPARAM)myCue->c_str());
	}

	void Edit::multiline(Bool v) {
		myMultiline = v;
		if (created())
			WARNING(L"Setting multiline after creation is not yet implemented.");
	}

	Bool Edit::multiline() {
		return myMultiline;
	}

	void Edit::removeLastWord() {
		Selection s = selected();
		if (s.end == 0)
			return;

		Str *t = text();
		s.end = min(s.end, t->peekLength());
		nat removeStart = s.end - 1;
		while (removeStart > 0) {
			if (t->c_str()[removeStart] == ' ')
				break;
			removeStart--;
		}

		text(*t->substr(t->begin(), t->posIter(removeStart)) + t->substr(t->posIter(s.end), t->end()));
		selected(Selection(removeStart));
	}

}
