#include "stdafx.h"
#include "Menu.h"
#include "Exception.h"
#include "App.h"
#include "Frame.h"
#include "Accelerators.h"

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

		repaint();
	}

	Menu &Menu::operator <<(Item *item) {
		push(item);
		return *this;
	}

	void Menu::repaint() {
		// Don't need to do anything here.
	}

	void Menu::onAddAccelerator(KeyChord, Fn<void> *, Handle) {}

	void Menu::onRemoveAccelerator(KeyChord) {}

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

	void Menu::addAccelerators(Accelerators *to) {
		for (Nat i = 0; i < items->count(); i++)
			items->at(i)->addAccelerators(to);
	}

	void Menu::removeAccelerators(Accelerators *to) {
		for (Nat i = 0; i < items->count(); i++)
			items->at(i)->addAccelerators(to);
	}


	Menu::Item::Item() : owner(null), enable(true) {}

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

	void Menu::Item::onShortcut() {
		if (enable)
			clicked();
	}

	Menu *Menu::Item::findMenu(Handle handle) const {
		return null;
	}

	Menu::Item *Menu::Item::findMenuItem(Handle handle) {
		if (this->handle == handle)
			return this;
		return null;
	}

	void Menu::Item::addAccelerators(Accelerators *) {}

	void Menu::Item::removeAccelerators(Accelerators *) {}

	Menu::Separator::Separator() {}

	Menu::WithTitle::WithTitle(MnemonicStr title) : myTitle(title) {}

	Menu::WithTitle::WithTitle(MnemonicStr title, KeyChord shortcut) : myTitle(title), myShortcut(shortcut) {}

	Str *Menu::WithTitle::findTitle() {
		Str *t = myTitle.win32Mnemonic();
		if (myShortcut.any())
			t = TO_S(t, t << S("\t") << myShortcut);
		return t;
	}

	void Menu::WithTitle::shortcut(KeyChord s) {
		if (owner) {
			if (myShortcut.any())
				owner->onRemoveAccelerator(myShortcut);

			if (s.any())
				owner->onAddAccelerator(s, fnPtr<void, Menu::Item>(engine(), &Menu::Item::onShortcut, this), handle);
		}

		myShortcut = s;

#if defined(GUI_WIN32)
		title(myTitle);
#endif
	}

	void Menu::WithTitle::setupShortcut() {
		if (owner && myShortcut.any())
			owner->onAddAccelerator(myShortcut, fnPtr<void, Menu::Item>(engine(), &Menu::Item::onShortcut, this), handle);
	}

	void Menu::WithTitle::addAccelerators(Accelerators *to) {
		if (myShortcut.any())
			to->add(myShortcut, fnPtr<void, Menu::Item>(engine(), &Menu::Item::onShortcut, this), handle);
	}

	void Menu::WithTitle::removeAccelerators(Accelerators *to) {
		if (myShortcut.any())
			to->remove(myShortcut);
	}

	Menu::Text::Text(MnemonicStr title) : WithTitle(title) {}

	Menu::Text::Text(MnemonicStr title, Fn<void> *callback) : WithTitle(title), onClick(callback) {}

	Menu::Text::Text(MnemonicStr title, KeyChord s) : WithTitle(title, s) {}

	Menu::Text::Text(MnemonicStr title, KeyChord s, Fn<void> *callback) : WithTitle(title, s), onClick(callback) {}

	void Menu::Text::clicked() {
		if (onClick)
			onClick->call();
	}

	Menu::Check::Check(MnemonicStr title) : WithTitle(title) {}

	Menu::Check::Check(MnemonicStr title, Fn<void, Bool> *callback) : WithTitle(title), onClick(callback) {}

	Menu::Check::Check(MnemonicStr title, Fn<void, Bool> *callback, Bool checked)
		: WithTitle(title), onClick(callback), myChecked(checked) {}

	Menu::Check::Check(MnemonicStr title, KeyChord s) : WithTitle(title, s) {}

	Menu::Check::Check(MnemonicStr title, KeyChord s, Fn<void, Bool> *callback)
		: WithTitle(title, s), onClick(callback) {}

	Menu::Check::Check(MnemonicStr title, KeyChord s, Fn<void, Bool> *callback, Bool checked)
		: WithTitle(title, s), onClick(callback), myChecked(checked) {}

	Menu::Submenu::Submenu(MnemonicStr title, PopupMenu *menu) : WithTitle(title), myMenu(menu) {
		if (menu->inside)
			throw new (this) GuiError(S("A popup menu may only be attached at a single place at a time."));
		menu->inside = this;
	}

	Menu *Menu::Submenu::findMenu(Handle handle) const {
		if (myMenu)
			return myMenu->findMenu(handle);
		return null;
	}

	Menu::Item *Menu::Submenu::findMenuItem(Handle handle) {
		if (Item *i = WithTitle::findMenuItem(handle))
			return i;
		if (myMenu)
			return myMenu->findMenuItem(handle);
		return null;
	}

	void Menu::Submenu::addAccelerators(Accelerators *to) {
		WithTitle::addAccelerators(to);
		if (myMenu)
			myMenu->addAccelerators(to);
	}

	void Menu::Submenu::removeAccelerators(Accelerators *to) {
		WithTitle::removeAccelerators(to);
		if (myMenu)
			myMenu->removeAccelerators(to);
	}

#ifdef GUI_WIN32

	void Menu::Item::enabled(Bool v) {
		enable = v;

		if (owner) {
			MENUITEMINFO info;
			info.cbSize = sizeof(info);
			info.fMask = MIIM_STATE;
			GetMenuItemInfo(owner->handle.menu(), id, TRUE, &info);
			if (enable)
				info.fState &= ~MF_DISABLED;
			else
				info.fState |= MF_DISABLED;
			SetMenuItemInfo(owner->handle.menu(), id, TRUE, &info);

			owner->repaint();
		}
	}

	void Menu::Separator::create() {
		AppendMenu(owner->handle.menu(), MF_SEPARATOR, 1, L"");
	}

	void Menu::WithTitle::title(MnemonicStr title) {
		myTitle = title;

		if (owner) {
			MENUITEMINFO info;
			info.cbSize = sizeof(info);
			info.fMask = MIIM_STRING;
			info.dwTypeData = (LPWSTR)findTitle()->c_str();
			SetMenuItemInfo(owner->handle.menu(), id, TRUE, &info);

			owner->repaint();
		}
	}

	void Menu::Text::create() {
		UINT flags = MF_STRING;
		if (!enable)
			flags |= MF_DISABLED;
		AppendMenu(owner->handle.menu(), flags, 1, findTitle()->c_str());
	}

	Bool Menu::Check::checked() {
		if (owner) {
			MENUITEMINFO info;
			info.cbSize = sizeof(info);
			info.fMask = MIIM_STATE;
			GetMenuItemInfo(owner->handle.menu(), id, TRUE, &info);

			myChecked = (info.fState & MF_CHECKED) != 0;
		}

		return myChecked;
	}

	void Menu::Check::checked(Bool v) {
		Bool changed = v != myChecked;
		myChecked = v;

		if (owner) {
			MENUITEMINFO info;
			info.cbSize = sizeof(info);
			info.fMask = MIIM_STATE;
			GetMenuItemInfo(owner->handle.menu(), id, TRUE, &info);
			UINT oldState = info.fState;
			if (v)
				info.fState |= MF_CHECKED;
			else
				info.fState &= ~MF_CHECKED;
			SetMenuItemInfo(owner->handle.menu(), id, TRUE, &info);

			if (info.fState != oldState) {
				owner->repaint();

				// We only get notifications when the checkbox is clicked, so we need to send a
				// notification ourselves now.
				if (onClick)
					onClick->call(v);
			}
		} else {
			if (changed && onClick)
				onClick->call(v);
		}
	}

	void Menu::Check::clicked() {
		Bool check = !checked();
		checked(check);
	}

	void Menu::Check::create() {
		UINT flags = MF_STRING;
		if (!enable)
			flags |= MF_DISABLED;
		if (myChecked)
			flags |= MF_CHECKED;
		AppendMenu(owner->handle.menu(), flags, 1, findTitle()->c_str());
	}

	void Menu::Submenu::create() {
		if (myMenu->parent)
			throw new (this) GuiError(S("This sub-menu is already used somewhere else!"));
		myMenu->parent = owner;

		UINT flags = MF_STRING | MF_POPUP;
		if (!enable)
			flags |= MF_DISABLED;
		AppendMenu(owner->handle.menu(), flags, (UINT_PTR)myMenu->handle.menu(), findTitle()->c_str());
	}

	void Menu::Submenu::destroy() {
		// Remove the submenu before we're destroyed to avoid us recursively destroying the menu.
		RemoveMenu(owner->handle.menu(), id, MF_BYPOSITION);

		WithTitle::destroy();
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

	PopupMenu::PopupMenu() : inside(null) {
		handle = CreatePopupMenu();
		setupMenu(handle.menu());
	}

	MenuBar::MenuBar() {
		handle = CreateMenu();
		setupMenu(handle.menu());
	}

	void MenuBar::repaint() {
		if (attachedTo) {
			if (attachedTo->created()) {
				DrawMenuBar(attachedTo->handle().hwnd());
			}
		}
	}

#endif
#ifdef GUI_GTK

	static void menuCallback(GtkWidget *src, gpointer engine) {
		Engine *e = (Engine *)engine;
		App *app = gui::app(*e);

		Menu::Item *item = app->findMenuItem(src);
		if (!item) {
			// Note: This happens for example when a Menu::Check is toggled before it is shown
			// and/or properly added to a window.
			// WARNING(L"Unknown menu item!");
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

	void Menu::Item::enabled(Bool v) {
		enable = v;

		if (handle != Handle()) {
			gtk_widget_set_sensitive(handle.widget(), v ? TRUE : FALSE);
		}
	}

	void Menu::Separator::create() {
		GtkWidget *created = gtk_separator_menu_item_new();
		g_object_ref_sink(created);
		handle = created;
		gtk_menu_shell_append(GTK_MENU_SHELL(owner->handle.widget()), created);
	}

	void Menu::WithTitle::title(MnemonicStr title) {
		myTitle = title;

		if (handle != Handle())
			gtk_menu_item_set_label(GTK_MENU_ITEM(handle.widget()), title.mnemonic()->utf8_str());
	}

	void Menu::Text::create() {
		GtkWidget *created = gtk_menu_item_new_with_mnemonic(myTitle.mnemonic()->utf8_str());
		g_object_ref_sink(created);
		gtk_widget_set_sensitive(created, enable ? TRUE : FALSE);
		handle = created;
		gtk_menu_shell_append(GTK_MENU_SHELL(owner->handle.widget()), created);
		connectMenu(created, engine());
	}

	Bool Menu::Check::checked() {
		if (handle != Handle()) {
			myChecked = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(handle.widget()));
		}

		return myChecked;
	}

	void Menu::Check::checked(Bool v) {
		Bool changed = v != myChecked;
		myChecked = v;

		if (handle != Handle()) {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(handle.widget()), v ? TRUE : FALSE);
		} else {
			// If it is not created, we need to send the notification ourselves.
			if (changed && onClick)
				onClick->call(v);
		}
	}

	void Menu::Check::clicked() {
		Bool check = checked();
		// checked(check);

		if (onClick)
			onClick->call(check);
	}

	void Menu::Check::create() {
		GtkWidget *created = gtk_check_menu_item_new_with_mnemonic(myTitle.mnemonic()->utf8_str());
		g_object_ref_sink(created);
		gtk_widget_set_sensitive(created, enable ? TRUE : FALSE);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(created), myChecked ? TRUE : FALSE);
		handle = created;
		gtk_menu_shell_append(GTK_MENU_SHELL(owner->handle.widget()), created);
		connectMenu(created, engine());
	}

	void Menu::Submenu::create() {
		GtkWidget *created = gtk_menu_item_new_with_mnemonic(myTitle.mnemonic()->utf8_str());
		g_object_ref_sink(created);
		gtk_widget_set_sensitive(created, enable ? TRUE : FALSE);
		handle = created;
		gtk_menu_shell_append(GTK_MENU_SHELL(owner->handle.widget()), created);
		connectMenu(created, engine());

		if (myMenu->parent)
			throw new (this) GuiError(S("This sub-menu is already used somewhere else!"));
		myMenu->parent = owner;
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(handle.widget()), myMenu->handle.widget());
	}

	void Menu::Submenu::destroy() {
		WithTitle::destroy();
	}


	PopupMenu::PopupMenu() : inside(null) {
		GtkWidget *created = gtk_menu_new();
		g_object_ref_sink(created);
		handle = created;
	}

	MenuBar::MenuBar() {
		GtkWidget *created = gtk_menu_bar_new();
		g_object_ref_sink(created);
		handle = created;
	}

	void MenuBar::repaint() {
		if (attachedTo) {
			gtk_widget_show_all(handle.widget());
		}
	}

#endif

	void PopupMenu::onAddAccelerator(KeyChord chord, Fn<void> *action, Handle handle) {
		if (inside && inside->owner) {
			inside->owner->onAddAccelerator(chord, action, handle);
		}
	}

	void PopupMenu::onRemoveAccelerator(KeyChord chord) {
		if (inside && inside->owner) {
			inside->owner->onRemoveAccelerator(chord);
		}
	}

	void MenuBar::onAddAccelerator(KeyChord chord, Fn<void> *action, Handle handle) {
		if (attachedTo) {
			attachedTo->accelerators()->add(chord, action, handle);
		}
	}

	void MenuBar::onRemoveAccelerator(KeyChord chord) {
		if (attachedTo) {
			attachedTo->accelerators()->remove(chord);
		}
	}


}
