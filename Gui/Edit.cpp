#include "stdafx.h"
#include "Edit.h"
#include "Container.h"
#include "Core/Convert.h"
#include "Core/Utf.h"

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
	bool Edit::create(ContainerBase *parent, nat id) {
		DWORD flags = editFlags;
		if (myMultiline)
			flags |= ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN;

		if (!Window::createEx(WC_EDIT, flags, WS_EX_CLIENTEDGE, parent->handle().hwnd(), id))
			return false;

		// Update selection.
		selected(sel);

		// Cue banner.
		SendMessage(handle().hwnd(), EM_SETCUEBANNER, 0, (LPARAM)myCue->c_str());

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
			SendMessage(handle().hwnd(), EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
			sel.start = start;
			sel.end = end;
		}
		return sel;
	}

	void Edit::selected(Selection sel) {
		this->sel = sel;
		if (created())
			SendMessage(handle().hwnd(), EM_SETSEL, (WPARAM)sel.start, (LPARAM)sel.end);
	}

	Str *Edit::cue() {
		return myCue;
	}

	void Edit::cue(Str *s) {
		myCue = s;
		if (created())
			SendMessage(handle().hwnd(), EM_SETCUEBANNER, 0, (LPARAM)myCue->c_str());
	}

	Size Edit::minSize() {
		Size sz;
		if (myCue->any())
			sz = font()->stringSize(myCue);
		else
			sz = font()->stringSize(new (this) Str(S("AAAA")));
		sz.w += sz.h;
		sz.h *= 1.6f;
		return sz;
	}

#endif

#ifdef GUI_GTK
	bool Edit::create(ContainerBase *parent, nat id) {
		GtkWidget *edit;
		if (myMultiline) {
			edit = gtk_text_view_new();
			// placeholder text is not supported...
		} else {
			edit = gtk_entry_new();
			gtk_entry_set_placeholder_text(GTK_ENTRY(edit), myCue->utf8_str());
		}
		initWidget(parent, edit);

		// Set the initial text.
		text(Window::text());

		// Apply selection.
		selected(sel);

		return true;
	}

	Str *Edit::text() {
		if (created()) {
			GtkWidget *edit = handle().widget();
			if (GTK_IS_ENTRY(edit)) {
				const gchar *buf = gtk_entry_get_text(GTK_ENTRY(edit));
				Window::text(new (this) Str(toWChar(engine(), buf)));
			} else if (GTK_IS_TEXT_VIEW(edit)) {
				GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(edit));
				GtkTextIter begin, end;
				gtk_text_buffer_get_start_iter(buffer, &begin);
				gtk_text_buffer_get_end_iter(buffer, &end);
				char *buf = gtk_text_buffer_get_text(buffer, &begin, &end, TRUE);
				Window::text(new (this) Str(toWChar(engine(), buf)));
				g_free(buf);
			}
		}
		return Window::text();
	}

	void Edit::text(Str *str) {
		if (created()) {
			GtkWidget *edit = handle().widget();
			if (GTK_IS_ENTRY(edit)) {
				gtk_entry_set_text(GTK_ENTRY(edit), str->utf8_str());
			} else {
				GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(edit));
				gtk_text_buffer_set_text(buffer, str->utf8_str(), -1);
			}
		}
		Window::text(str);
	}

	Selection Edit::selected() {
		if (created()) {
			GtkWidget *edit = handle().widget();
			if (GTK_IS_ENTRY(edit)) {
				gint start, end;
				gtk_editable_get_selection_bounds(GTK_EDITABLE(edit), &start, &end);
				sel = Selection(Nat(start), Nat(end));
			} else {
				GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(edit));
				GtkTextMark *insert = gtk_text_buffer_get_insert(buffer);
				GtkTextMark *bound = gtk_text_buffer_get_selection_bound(buffer);

				GtkTextIter begin, end;
				gtk_text_buffer_get_iter_at_mark(buffer, &end, insert);
				gtk_text_buffer_get_iter_at_mark(buffer, &begin, bound);

				sel = Selection(gtk_text_iter_get_offset(&begin),
								gtk_text_iter_get_offset(&end));
			}
		}
		return sel;
	}

	void Edit::selected(Selection sel) {
		if (created()) {
			GtkWidget *edit = handle().widget();
			// Note: This could be a bit wrong... We're working with character offsets, not
			// codepoints. They are usually the same in UTF-16, but as we haven't really specified
			// what the integers represent inside Selection (characters is easier to find out than
			// offsets), this is fine for now.
			if (GTK_IS_ENTRY(edit)) {
				gtk_editable_select_region(GTK_EDITABLE(edit), sel.start, sel.end);
			} else {
				GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(edit));

				GtkTextIter begin, end;
				gtk_text_buffer_get_iter_at_offset(buffer, &begin, sel.start);
				gtk_text_buffer_get_iter_at_offset(buffer, &end, sel.end);

				// 'begin' is the fixed portion of the range.
				gtk_text_buffer_select_range(buffer, &end, &begin);
			}
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

	Size Edit::minSize() {
		gint w = 0, h = 0;

		if (created()) {
			gtk_widget_get_preferred_width(handle().widget(), &w, NULL);
			gtk_widget_get_preferred_height(handle().widget(), &h, NULL);
		}

		return Size(Float(w), Float(h));
	}

#endif

}
