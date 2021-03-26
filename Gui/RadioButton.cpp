#include "stdafx.h"
#include "RadioButton.h"
#include "Container.h"
#include "Exception.h"
#include "GtkSignal.h"

namespace gui {

	RadioGroup::RadioGroup() {
		buttons = new (this) WeakSet<RadioButton>();
	}

	void RadioGroup::add(RadioButton *button) {
		buttons->put(button);
	}

	void RadioGroup::remove(RadioButton *button) {
		buttons->remove(button);
	}

	void RadioGroup::clear() {
		WeakSet<RadioButton>::Iter i = buttons->iter();
		while (RadioButton *button = i.next()) {
			button->checked(false);
		}
	}

	void RadioGroup::clearExcept(RadioButton *except) {
		WeakSet<RadioButton>::Iter i = buttons->iter();
		while (RadioButton *button = i.next()) {
			if (button != except)
				button->checked(false);
		}
	}

	RadioButton *RadioGroup::pickCreated() {
		WeakSet<RadioButton>::Iter i = buttons->iter();
		while (RadioButton *button = i.next()) {
			if (button->created())
				return button;
		}

		return null;
	}


	RadioButton::RadioButton(Str *title) {
		text(title);
		group(new (this) RadioGroup());
	}

	RadioButton::RadioButton(Str *title, Fn<void> *activate) {
		text(title);
		group(new (this) RadioGroup());
		onActivate = activate;
	}

	RadioButton::RadioButton(Str *title, Fn<void, Bool> *change) {
		text(title);
		group(new (this) RadioGroup());
		onChange = change;
	}

	RadioButton::RadioButton(Str *title, Nat group) : autoGroup(group) {
		text(title);
	}

	RadioButton::RadioButton(Str *title, Nat group, Fn<void> *activate) : autoGroup(group) {
		text(title);
		onActivate = activate;
	}

	RadioButton::RadioButton(Str *title, Nat group, Fn<void, Bool> *change) : autoGroup(group) {
		text(title);
		onChange = change;
	}

	void RadioButton::change(Bool to) {
		if (to && myGroup)
			myGroup->clearExcept(this);

		if (onChange)
			onChange->call(to);
		if (to && onActivate)
			onActivate->call();
	}

	void RadioButton::group(RadioGroup *group) {
		if (myGroup)
			myGroup->remove(this);

#ifdef GUI_GTK
		if (created()) {
			GtkRadioButton *button = null;
			if (RadioButton *b = myGroup->pickCreated())
				button = GTK_RADIO_BUTTON(b->handle().widget());
			if (button)
				gtk_radio_button_join_group(GTK_RADIO_BUTTON(handle().widget()), button);
			else
				// Else: create a new group for us.
				gtk_radio_button_set_group(GTK_RADIO_BUTTON(handle().widget()), NULL);
		}
#endif

		myGroup = group;
		myGroup->add(this);
	}

	RadioGroup *RadioButton::group() {
		if (myGroup)
			return myGroup;
		else
			throw new (this) GuiError(S("Can not get an automatically created group before the radio button is created."));
	}

	void RadioButton::findGroup(ContainerBase *parent) {
		if (!myGroup) {
			myGroup = parent->radioGroup(autoGroup);
			myGroup->add(this);
		}
	}

#ifdef GUI_WIN32

	bool RadioButton::create(ContainerBase *parent, nat id) {
		findGroup(parent);

		DWORD flags = controlFlags | BS_RADIOBUTTON;
		if (Window::createEx(WC_BUTTON, flags, 0, parent->handle().hwnd(), id)) {
			SendMessage(handle().hwnd(), BM_SETCHECK, isChecked ? BST_CHECKED : BST_UNCHECKED, 0);
			return true;
		}
		return false;
	}

	bool RadioButton::onCommand(nat id) {
		if (id == BN_CLICKED) {
			if (!checked()) {
				checked(true);
			}
			return true;
		}

		return false;
	}

	Size RadioButton::minSize() {
		Size sz = font()->stringSize(text());
		sz.w += GetSystemMetrics(SM_CXMENUCHECK) + GetSystemMetrics(SM_CXEDGE);
		sz.h = max(sz.h, float(GetSystemMetrics(SM_CYMENUCHECK)));
		return sz;
	}

	Bool RadioButton::checked() {
		return isChecked;
	}

	void RadioButton::checked(Bool v) {
		if (v == isChecked)
			return;

		isChecked = v;
		change(v);

		if (created()) {
			SendMessage(handle().hwnd(), BM_SETCHECK, isChecked ? BST_CHECKED : BST_UNCHECKED, 0);
		}
	}

#endif
#ifdef GUI_GTK

	bool RadioButton::create(ContainerBase *parent, nat id) {
		findGroup(parent);

		GtkWidget *group = null;
		if (myGroup)
			if (RadioButton *b = myGroup->pickCreated())
				group = b->handle().widget();

		GtkWidget *button;
		if (group)
			button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(group), text()->utf8_str());
		else
			button = gtk_radio_button_new_with_label(NULL, text()->utf8_str());

		initWidget(parent, button);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), isChecked);
		Signal<void, RadioButton>::Connect<&RadioButton::toggled>::to(button, "toggled", engine());
		return true;
	}

	void RadioButton::toggled() {
		change(checked());
	}

	void RadioButton::text(Str *text) {
		if (created()) {
			GtkWidget *widget = gtk_bin_get_child(GTK_BIN(handle().widget()));
			gtk_label_set_text(GTK_LABEL(widget), text->utf8_str());
		}
		Window::text(text);
	}

	GtkWidget *RadioButton::fontWidget() {
		return gtk_bin_get_child(GTK_BIN(handle().widget()));
	}

	Size RadioButton::minSize() {
		gint w = 0, h = 0;

		if (created()) {
			gtk_widget_get_preferred_width(handle().widget(), &w, NULL);
			gtk_widget_get_preferred_height(handle().widget(), &h, NULL);
		}

		return Size(Float(w), Float(h));
	}

	Bool RadioButton::checked() {
		if (created()) {
			isChecked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(handle().widget()));
		}

		return isChecked;
	}

	void RadioButton::checked(Bool v) {
		isChecked = v;
		if (created()) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(handle().widget()), isChecked);
		}
	}

#endif

}
