#pragma once
#include "Window.h"

namespace stormgui {

	/**
	 * Selection for the edit control.
	 */
	class Selection {
		STORM_VALUE;
	public:
		STORM_CTOR Selection();
		STORM_CTOR Selection(Nat pos);
		STORM_CTOR Selection(Nat start, Nat end);

		STORM_VAR Nat start;
		STORM_VAR Nat end;
	};

	/**
	 * Text input field. Either singleline or multiline.
	 */
	class Edit : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR Edit();

		// Key handling for implementing Ctrl+Backspace.
		virtual Bool STORM_FN onChar(Nat code);

		// Implement the 'return' as a confirmation.
		virtual Bool STORM_FN onKey(Bool pressed, Nat code);

		// Manipulate the selection.
		Selection STORM_FN selected();
		void STORM_SETTER selected(Selection sel);

		// Called when return is pressed.
		STORM_VAR Auto<FnPtr<void, Par<Edit>>> onReturn;

	protected:
		virtual bool create(HWND parent, nat id);

	private:
		// Our selection.
		Selection sel;

		// Remove the last word of our contents.
		void removeLastWord();
	};

}
