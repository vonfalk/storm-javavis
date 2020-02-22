#pragma once
#include "Core/Array.h"
#include "Core/Fn.h"
#include "Handle.h"

namespace gui {

	class PopupMenu;

	/**
	 * A generic drop-down menu that is either shown as a sub-menu to the main window menu, or as a
	 * stand-alone popup menu (e.g. when the user right-clicks something).
	 */
	class Menu : public ObjectOn<Ui> {
		STORM_CLASS;
		friend class Frame;
	public:
		// Create. No parent.
		STORM_CTOR Menu();

		// Destroy the menu.
		virtual ~Menu();

		/**
		 * A single menu item.
		 *
		 * Abstract class, instantiate one of the subclasses instead.
		 */
		class Item : public ObjectOn<Ui> {
			STORM_ABSTRACT_CLASS;
			friend class Menu;
		public:
			// Create.
			STORM_CTOR Item();

			// Called whenever we're attached to a parent. This is when we create ourselves.
			void STORM_FN attached(Menu *to, Nat id);

			// Called when the item was clicked.
			virtual void STORM_FN clicked();

			// Find a sub-menu from its handle.
			virtual MAYBE(Menu *) findMenu(Handle handle) const;

			// Find a menu item from its handle (only meaningful on Gtk+).
			virtual MAYBE(Item *) findMenuItem(Handle handle);

		protected:
			// Owning menu so that we can propagate updates.
			MAYBE(Menu *) owner;

			// Our ID inside 'owner'.
			Nat id;

			// Handle to the item we contain.
			Handle handle;

			// Called to create the element in here.
			virtual void STORM_FN create() ABSTRACT;

			// Destroy the element in here. Called by 'Menu'.
			virtual void STORM_FN destroy();
		};

		/**
		 * A menu item acting as a separator.
		 */
		class Separator : public Item {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Separator();

		protected:
			// Create our element.
			virtual void STORM_FN create();
		};

		/**
		 * A single menu item containing text.
		 */
		class Text : public Item {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Text(Str *title);
			STORM_CTOR Text(Str *title, Fn<void> *fn);

			// Get/set the title.
			Str *STORM_FN title() const { return myTitle; }
			void STORM_ASSIGN title(Str *title);

			// Callback.
			MAYBE(Fn<void> *) onClick;

			// Called when the item was clicked.
			virtual void STORM_FN clicked();

		protected:
			// Create our element.
			virtual void STORM_FN create();

			// Title.
			Str *myTitle;
		};

		/**
		 * A submenu.
		 */
		class Submenu : public Text {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Submenu(Str *title, PopupMenu *menu);

			// Find a sub-menu from its handle.
			virtual MAYBE(Menu *) findMenu(Handle handle) const;

			// Find a menu item from its handle (only meaningful on Gtk+).
			virtual MAYBE(Item *) findMenuItem(Handle handle);

		protected:
			// Create our element.
			virtual void STORM_FN create();

			// Destroy the element in here. Called by 'Menu'.
			virtual void STORM_FN destroy();

		private:
			// Submenu.
			PopupMenu *myMenu;
		};


		// Add a menu item.
		void STORM_FN push(Item *item);
		Menu &STORM_FN operator <<(Item *item);

		// Access the items.
		Nat STORM_FN count() const { return items->count(); }

		// Get an item.
		Item *STORM_FN operator [](Nat id) const { return items->at(id); }

		// Find a sub-menu from its handle.
		MAYBE(Menu *) findMenu(Handle handle);

		// Find a menu-item from its handle (only meaningful on Gtk+).
		MAYBE(Menu::Item *) findMenuItem(Handle handle);

	private:
		// Parent menu, if any.
		MAYBE(Menu *) parent;

		// Items.
		Array<Item *> *items;

	protected:
		// The underlying widget/os resource.
		Handle handle;
	};

	/**
	 * A popup menu, that can be used as a sub-menu to the main menu bar of a window, or as a
	 * stand-alone popup menu.
	 */
	class PopupMenu : public Menu {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR PopupMenu();
	};

	/**
	 * A menu-bar suitable for placement inside a Frame.
	 *
	 * A menu bar may only be attached to one frame at any given time.
	 */
	class MenuBar : public Menu {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR MenuBar();
	};

}
