#include "stdafx.h"
#include "Menu.h"
#include "Exception.h"
#include "App.h"

namespace gui {

	Menu::Menu() : parent(null), items(new (engine()) Array<Item *>()) {}

	Menu::~Menu() {
		if (items) {
			// Note: This is slightly dangerous during shutdown...
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
		handle = Handle();
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

	Menu *Menu::findMenu(Handle handle) {
		if (handle == this->handle)
			return this;

		for (Nat i = 0; i < items->count(); i++) {
			if (Menu *m = items->at(i)->findMenu(handle))
				return m;
		}

		return null;
	}

	Menu::Item *Menu::findMenuItem(Handle handle) {
		for (Nat i = 0; i < items->count(); i++) {
			if (Item *item = items->at(i)->findMenuItem(handle))
				return item;
		}
		return null;
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
		// No cleanup is needed on Win32.

#ifdef GUI_GTK
		if (handle != Handle())
			g_object_unref(handle.widget());
		handle = Handle();
#endif
	}

	void Menu::Item::clicked() {}

	Menu *Menu::Item::findMenu(Handle handle) const {
		return null;
	}

	Menu::Item *Menu::Item::findMenuItem(Handle handle) {
		if (this->handle == handle)
			return this;
		return null;
	}

	Menu::Separator::Separator() {}


	Menu::Text::Text(Str *title) : myTitle(title) {}

	Menu::Text::Text(Str *title, Fn<void> *callback) : myTitle(title), onClick(callback) {}

	void Menu::Text::clicked() {
		if (onClick)
			onClick->call();
	}


	Menu::Submenu::Submenu(Str *title, PopupMenu *menu) : Text(title), myMenu(menu) {}

	Menu *Menu::Submenu::findMenu(Handle handle) const {
		if (myMenu)
			return myMenu->findMenu(handle);
		return null;
	}

	Menu::Item *Menu::Submenu::findMenuItem(Handle handle) {
		if (Item *i = Text::findMenuItem(handle))
			return i;
		if (myMenu)
			return myMenu->findMenuItem(handle);
		return null;
	}

#ifdef GUI_WIN32

	void Menu::Separator::create() {
		AppendMenu(owner->handle.menu(), MF_SEPARATOR, 1, L"");
	}

	void Menu::Text::title(Str *title) {
		myTitle = title;

		MENUITEMINFO info;
		info.cbSize = sizeof(info);
		info.fMask = MIIM_STRING;
		info.dwTypeData = (LPWSTR)myTitle->c_str();
		SetMenuItemInfo(owner->handle.menu(), id, FALSE, &info);
	}

	void Menu::Text::create() {
		AppendMenu(owner->handle.menu(), MF_STRING, 1, myTitle->c_str());
	}

	void Menu::Submenu::create() {
		if (myMenu->parent)
			throw new (this) GuiError(S("This sub-menu is already used somewhere else!"));
		myMenu->parent = owner;

		AppendMenu(owner->handle.menu(), MF_STRING | MF_POPUP, (UINT_PTR)myMenu->handle.menu(), myTitle->c_str());
	}

	void Menu::Submenu::destroy() {
		// Remove the submenu before we're destroyed to avoid us recursively destroying the menu.
		RemoveMenu(owner->handle.menu(), id, MF_BYPOSITION);

		Text::destroy();
	}

	static void setupMenu(HMENU menu) {
		MENUINFO info;
		info.cbSize = sizeof(info);
		info.fMask = MIM_STYLE;
		// Note: We don't need to set MNS_NOTIFYBYPOS for sub-menus, but I don't think it hurts.
		// Note: Since there is a MNS_MODELESS flag, it appears that the message loop is blocked while
		// a menu is being interacted with. We might want to circumvent that...
		info.dwStyle = MNS_NOTIFYBYPOS;
		SetMenuInfo(menu, &info);
	}

	PopupMenu::PopupMenu() {
		handle = CreatePopupMenu();
		setupMenu(handle.menu());
	}

	MenuBar::MenuBar() {
		handle = CreateMenu();
		setupMenu(handle.menu());
	}

#endif
#ifdef GUI_GTK

	static void menuCallback(GtkWidget *src, gpointer engine) {
		Engine *e = (Engine *)engine;
		App *app = gui::app(*e);

		Menu::Item *item = app->findMenuItem(src);
		if (!item) {
			WARNING(L"Unknown menu item!");
			return;
		}

		try {
			return item->clicked();
		} catch (const storm::Exception *e) {
			PLN(L"Unhandled exception in window thread:\n" << e->message());
		} catch (const ::Exception &e) {
			PLN(L"Unhandled exception in window thread:\n" << e);
		} catch (...) {
			PLN(L"Unhandled exception in window thread: <unknown>");
		}
	}

		// Connect to a signal associated with a Menu.
	static void connectMenu(GtkWidget *to, Engine &e) {
		g_signal_connect(to, "activate", (GCallback)&menuCallback, &e);
	}

	void Menu::Separator::create() {
		GtkWidget *created = gtk_separator_menu_item_new();
		g_object_ref_sink(created);
		handle = created;
		gtk_menu_shell_append(GTK_MENU_SHELL(owner->handle.widget()), created);
	}

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
		connectMenu(created, engine());
	}

	void Menu::Submenu::create() {
		Text::create();

		if (myMenu->parent)
			throw new (this) GuiError(S("This sub-menu is already used somewhere else!"));
		myMenu->parent = owner;
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(handle.widget()), myMenu->handle.widget());
	}

	void Menu::Submenu::destroy() {
		Text::destroy();
	}


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
