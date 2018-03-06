#pragma once
#include "Window.h"
#include "Core/Fn.h"

namespace gui {

	/**
	 * Selection for the edit control.
	 *
	 * TODO: Iterators?
	 */
	class Selection {
		STORM_VALUE;
	public:
		STORM_CTOR Selection();
		STORM_CTOR Selection(Nat pos);
		STORM_CTOR Selection(Nat start, Nat end);

		Nat start;
		Nat end;
	};

	Str *STORM_FN toS(EnginePtr e, Selection s);

	/**
	 * Text input field.
	 */
	class Edit : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR Edit();
		STORM_CTOR Edit(Str *cue);

#ifdef GUI_WIN32
		// Key handling for implementing Ctrl+Backspace.
		virtual Bool STORM_FN onChar(Nat code);
#endif

		// Implement the 'return' as a confirmation.
		virtual Bool STORM_FN onKey(Bool pressed, key::Key code, mod::Modifiers mod);

#ifdef GUI_GTK
		// Get/set text.
		virtual const Str *STORM_FN text();
		virtual void STORM_FN text(Str *text);
#endif

		// Manipulate the selection.
		Selection STORM_FN selected();
		void STORM_ASSIGN selected(Selection sel);

		// Called when return is pressed.
		MAYBE(Fn<void, Edit *> *) onReturn;

		// Cue banner.
		void STORM_ASSIGN cue(Str *t);
		Str *STORM_FN cue();

		// Multiline. You can currently only set multiline before the control is created.
		void STORM_ASSIGN multiline(Bool v);
		Bool STORM_FN multiline();

	protected:
		// Create.
		virtual bool create(Container *parent, nat id);

	private:
		// Our selection.
		Selection sel;

		// Cue banner.
		Str *myCue;

		// Multiline?
		Bool myMultiline;

		// Remove the last word of our contents.
		void removeLastWord();
	};

}
