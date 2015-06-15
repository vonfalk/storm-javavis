#include "stdafx.h"
#include "Edit.h"

namespace stormgui {

	Selection::Selection() : start(0), end(0) {}

	Selection::Selection(Nat s) : start(s), end(s) {}

	Selection::Selection(Nat s, Nat e) : start(s), end(e) {}


	Edit::Edit() {
		onReturn = NullFn<void, Par<Edit>>::create(engine());
	}

	Edit::Edit(Par<Str> cue) {
		myCue = cue->v;
	}

	bool Edit::create(HWND parent, nat id) {
		if (!Window::createEx(WC_EDIT, controlFlags, WS_EX_CLIENTEDGE, parent, id))
			return false;

		// Update selection.
		selected(sel);

		// Cue banner.
		SendMessage(handle(), EM_SETCUEBANNER, 0, (LPARAM)myCue.c_str());

		return true;
	}

	Bool Edit::onChar(Nat code) {
		if (code == 127 && pressed(VK_CONTROL)) {
			removeLastWord();
			return true;
		}
		return false;
	}

	Bool Edit::onKey(Bool pressed, Nat code) {
		if (pressed && code == VK_RETURN) {
			if (onReturn) {
				onReturn->call(this);
				return true;
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
		return CREATE(Str, this, myCue);
	}

	void Edit::cue(Par<Str> s) {
		myCue = s->v;
		if (created())
			SendMessage(handle(), EM_SETCUEBANNER, 0, (LPARAM)myCue.c_str());
	}

	void Edit::removeLastWord() {
		Selection s = selected();
		if (s.end == 0)
			return;

		String t = cText();
		s.end = min(s.end, t.size());
		nat removeStart = s.end - 1;
		while (removeStart > 0) {
			if (t[removeStart] == ' ')
				break;
			removeStart--;
		}

		cText(t.left(removeStart) + t.mid(s.end));
		selected(Selection(removeStart));
	}

}
