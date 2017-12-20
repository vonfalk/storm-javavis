#include "stdafx.h"
#include "Edit.h"
#include "Core/Convert.h"

namespace gui {

	Selection::Selection() : start(0), end(0) {}

	Selection::Selection(Nat s) : start(s), end(s) {}

	Selection::Selection(Nat s, Nat e) : start(s), end(e) {}

	Str *toS(EnginePtr e, Selection s) {
		StrBuf *out = new (e.v) StrBuf();
		*out << S("{") << s.start << S(", ") << s.end << S("}");
		return out->toS();
	}

	Edit::Edit() {
		myMultiline = false;
		onReturn = null;
		myCue = new (this) Str(L"");
	}

	Edit::Edit(Str *cue) {
		myMultiline = false;
		myCue = cue;
	}

	void Edit::multiline(Bool v) {
		if (created()) {
			WARNING(L"Can not set multiline after creation!");
			return;
		}
		myMultiline = v;
	}

	Bool Edit::multiline() {
		return myMultiline;
	}

	void Edit::removeLastWord() {
		Selection s = selected();
		if (s.end == 0)
			return;

		const Str *t = text();
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

	Bool Edit::onKey(Bool down, key::Key code, Modifiers mod) {
		if (down && code == key::ret) {
			if (!myMultiline || (mod & mod::ctrl)) {
				if (onReturn) {
					onReturn->call(this);
					return true;
				}
			}
		}
		return false;
	}

#ifdef GUI_WIN32
	bool Edit::create(Container *parent, nat id) {
		DWORD flags = editFlags;
		if (myMultiline)
			flags |= ES_MULTILINE | WS_VSCROLL | ES_WANTRETURN;

		if (!Window::createEx(WC_EDIT, flags, WS_EX_CLIENTEDGE, parent->handle().hwnd(), id))
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

#endif

#ifdef GUI_GTK
	bool Edit::create(Container *parent, nat id) {
		if (myMultiline)
			TODO(L"Implement multiline edit control.");
		GtkWidget *edit = gtk_entry_new();
		gtk_entry_set_placeholder_text(GTK_ENTRY(edit), myCue->utf8_str());
		gtk_editable_select_region(GTK_EDITABLE(edit), sel.start, sel.end);
		initWidget(parent, edit);
		return true;
	}

	const Str *Edit::text() {
		if (created()) {
			const gchar *buf = gtk_entry_get_text(GTK_ENTRY(handle().widget()));
			Window::text(new (this) Str(toWChar(engine(), buf)));
		}
		return Window::text();
	}

	void Edit::text(Str *str) {
		if (created()) {
			gtk_entry_set_text(GTK_ENTRY(handle().widget()), str->utf8_str());
		}
		Window::text(str);
	}

	Selection Edit::selected() {
		if (created()) {
			gint start, end;
			gtk_editable_get_selection_bounds(GTK_EDITABLE(handle().widget()), &start, &end);
			sel = Selection(Nat(start), Nat(end));
		}
		return sel;
	}

	void Edit::selected(Selection sel) {
		if (created()) {
			gtk_editable_select_region(GTK_EDITABLE(handle().widget()), sel.start, sel.end);
		}
		this->sel = sel;
	}

	Str *Edit::cue() {
		return myCue;
	}

	void Edit::cue(Str *s) {
		if (created()) {
			gtk_entry_set_placeholder_text(GTK_ENTRY(handle().widget()), s->utf8_str());
		}
		myCue = s;
	}

#endif

}
