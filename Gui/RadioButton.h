#pragma once
#include "Window.h"
#include "Core/Fn.h"
#include "Core/WeakSet.h"

namespace gui {

	class RadioButton;

	/**
	 * A group of radio buttons.
	 */
	class RadioGroup : public ObjectOn<Ui> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR RadioGroup();

		// Add/remove elements.
		void STORM_FN add(RadioButton *button);
		void STORM_FN remove(RadioButton *button);

		// Clear the entire group.
		void STORM_FN clear();

		// Clear all except one.
		void STORM_FN clearExcept(RadioButton *button);

		// Pick an element that has been created.
		MAYBE(RadioButton *) pickCreated();

	private:
		// A weak set of all the contained radio buttons.
		WeakSet<RadioButton> *buttons;
	};


	/**
	 * Radio button.
	 *
	 * Note: If no radio button in a group is set to active, the system may select one to make
	 * active (e.g. Gtk+ does not allow none of the radio buttons to be pressed, this is possible to
	 * fix if we ever need it by having a hidden radio button somewhere).
	 */
	class RadioButton : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR RadioButton(Str *title);
		STORM_CTOR RadioButton(Str *title, Fn<void> *onActivate);
		STORM_CTOR RadioButton(Str *title, Fn<void, Bool> *onChange);

		// Automatically manage groups during creation. Group IDs are local within each container.
		// These are intended to be used with the layout syntax.
		STORM_CTOR RadioButton(Str *title, Nat groupId);
		STORM_CTOR RadioButton(Str *title, Nat groupId, Fn<void> *onActivate);
		STORM_CTOR RadioButton(Str *title, Nat groupId, Fn<void, Bool> *onChange);

		// Changed callback. Called whenever this radio button changes state.
		MAYBE(Fn<void, Bool> *) onChange;

		// Activated callback. Called whenever this radio button becomes active.
		MAYBE(Fn<void> *) onActivate;

		// Change the group this button belongs to.
		void STORM_ASSIGN group(RadioGroup *g);

		// Get the group.
		RadioGroup *STORM_FN group();

#ifdef GUI_WIN32
		virtual bool onCommand(nat id);
#endif
#ifdef GUI_GTK
		using Window::text;
		void text(Str *text);

		virtual GtkWidget *fontWidget();

		// Toggled signal.
		void toggled();
#endif

		// Checked?
		Bool STORM_FN checked();

		// Set checked.
		void STORM_ASSIGN checked(Bool check);

		// Minimum size.
		virtual Size STORM_FN minSize();

	protected:
		virtual bool create(ContainerBase *parent, nat id);

	private:
		// Group this radio button belongs to. Always contains something. If null, then 'autoGroup'
		// is used to find a group at a later time.
		MAYBE(RadioGroup *) myGroup;

		// If 'myGroup' is null, then we will use this to find a group.
		Nat autoGroup;

		// Find a group if necessary.
		void findGroup(ContainerBase *parent);

		// Change the state of this button.
		void change(Bool newState);

		// Checked?
		Bool isChecked;
	};

}
