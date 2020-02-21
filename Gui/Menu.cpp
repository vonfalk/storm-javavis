#include "stdafx.h"
#include "Menu.h"
#include "Exception.h"

namespace gui {

	Menu::Menu() : parent(null), items(new (engine()) Array<Item *>()) {}

	Menu::~Menu() {
		// Note: This is slightly dangerous...
		if (items) {
			for (Nat i = 0; i < items->count(); i++) {
				items->at(i)->destroy();
			}
		}

#ifdef GUI_WIN32
		if (handle != Handle())
			DestroyMenu(handle.menu());
#endif
#ifdef GUI_GTK
		if (handle != Handle())
			g_object_unref(handle.widget());
#endif
	}

	void Menu::push(Item *item) {
		Nat id = items->count();
		items->push(item);

		item->attached(this, id);
	}

	Menu &Menu::operator <<(Item *item) {
		push(item);
		return *this;
	}


	Menu::Item::Item() : owner(null) {}

	void Menu::Item::attached(Menu *to, Nat id) {
		if (owner)
			throw new (this) GuiError(S("This menu item is already attached to another menu!"));

		this->owner = to;
		this->id = id;

		create();
	}

	void Menu::Item::destroy() {
		if (handle != Handle())
			g_object_unref(handle.widget());
	}


	Menu::Separator::Separator() {}

	void Menu::Separator::create() {
		GtkWidget *created = gtk_separator_menu_item_new();
		g_object_ref_sink(created);
		handle = created;
		gtk_menu_shell_append(GTK_MENU_SHELL(owner->handle.widget()), created);
	}


	Menu::Text::Text(Str *title) : myTitle(title) {}

	void Menu::Text::title(Str *title) {
		myTitle = title;

		if (handle != Handle())
			gtk_menu_item_set_label(GTK_MENU_ITEM(handle.widget()), title->utf8_str());
	}

	void Menu::Text::create() {
		GtkWidget *created = gtk_menu_item_new_with_label(myTitle->utf8_str());
		g_object_ref_sink(created);
		handle = created;
		gtk_menu_shell_append(GTK_MENU_SHELL(owner->handle.widget()), created);
	}


	Menu::Submenu::Submenu(Str *title, PopupMenu *menu) : Text(title), myMenu(menu) {}

	void Menu::Submenu::create() {
		Text::create();

		if (myMenu->parent)
			throw new (this) GuiError(S("This sub-menu is already used somewhere else!"));
		myMenu->parent = owner;
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(handle.widget()), myMenu->handle.widget());
	}

	void Menu::Submenu::destroy() {
		// TODO: Make sure we don't accidentally destroy the submenu object.

		Text::destroy();
	}

#ifdef GUI_WIN32
#error "Implement me!"
#endif
#ifdef GUI_GTK

	PopupMenu::PopupMenu() {
		GtkWidget *created = gtk_menu_new();
		g_object_ref_sink(created);
		handle = created;
	}

	MenuBar::MenuBar() {
		GtkWidget *created = gtk_menu_bar_new();
		g_object_ref_sink(created);
		handle = created;
	}

#endif

}
