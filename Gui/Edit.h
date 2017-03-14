#pragma once
#include "Window.h"
#include "Core/Fn.h"

namespace gui {

	/**
	 * Selection for the edit control.
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

	/**
	 * Text input field.
	 */
	class Edit : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR Edit();
		STORM_CTOR Edit(Str *cue);

		// Key handling for implementing Ctrl+Backspace.
		virtual Bool STORM_FN onChar(Nat code);

		// Implement the 'return' as a confirmation.
		virtual Bool STORM_FN onKey(Bool pressed, Nat code);

		// Manipulate the selection.
		Selection STORM_FN selected();
		void STORM_SETTER selected(Selection sel);

		// Called when return is pressed.
		MAYBE(Fn<void, Edit *> *) onReturn;

		// Cue banner.
		void STORM_SETTER cue(Str *t);
		Str *STORM_FN cue();

		// Multiline. You can currently only set multiline before the control is created.
		void STORM_SETTER multiline(Bool v);
		Bool STORM_FN multiline();

	protected:
		virtual bool create(HWND parent, nat id);

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