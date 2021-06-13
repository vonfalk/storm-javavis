#include "stdafx.h"
#include "Accelerators.h"
#include "Core/Runtime.h"

namespace gui {

#if defined(GUI_GTK)

	static GcType staticType = {
		GcType::tFixed,
		null,
		null,
		sizeof(void *),
		1,
		{ 0 }
	};

	static void closureCallback(GtkAccelGroup *group, GObject *accel, guint keyval, GdkModifierType modifiers, void *data) {
		Accelerators *me = *(Accelerators **)data;
		me->dispatch(KeyChord(from_gtk(keyval), from_gtk(modifiers)));
	}

	static void closureDestroy(void *data, GClosure *c) {
		// No need.
	}


#endif

	Accelerators::Accelerators() : gtkData(null), staticData(null) {
		data = new (this) Map<KeyChord, Accel>();

#if defined(GUI_GTK)
		gtkData = gtk_accel_group_new();

		staticData = runtime::allocStaticRaw(engine(), &staticType);
		*(Accelerators **)staticData = this;
#endif
	}

	Accelerators::~Accelerators() {
#if defined(GUI_GTK)
		g_object_unref(gtkData);
#endif
	}

	void Accelerators::attach(Handle to) {
#if defined(GUI_GTK)
		// Seems like gtk_window_add_accel_group takes ownership of the data (not clearly
		// documented). Thus, we need to add a ref here.
		g_object_ref(gtkData);
		gtk_window_add_accel_group((GtkWindow *)to.widget(), (GtkAccelGroup *)gtkData);
#endif
	}


	void Accelerators::add(KeyChord chord, Fn<void> *call) {
		data->put(chord, Accel(call));

#if defined(GUI_GTK)
		// Note: Does not seem like we can share closure between keys.
		GClosure *closure = g_cclosure_new(G_CALLBACK(&closureCallback), staticData, &closureDestroy);
		gtk_accel_group_connect((GtkAccelGroup *)gtkData, to_gtk(chord.key), to_gtk(chord.modifiers), (GtkAccelFlags)0, closure);
#endif
	}

	void Accelerators::add(KeyChord chord, Fn<void> *call, Handle item) {
		data->put(chord, Accel(call));

#if defined(GUI_GTK)
		guint key = to_gtk(chord.key);
		GdkModifierType mods = to_gtk(chord.modifiers);
		gtk_widget_add_accelerator(item.widget(), "activate", (GtkAccelGroup *)gtkData, key, mods, GTK_ACCEL_VISIBLE);
#endif
	}

	void Accelerators::remove(KeyChord chord) {
		data->remove(chord);

#if defined(GUI_GTK)
		gtk_accel_group_disconnect_key((GtkAccelGroup *)gtkData, to_gtk(chord.key), to_gtk(chord.modifiers));
#endif
	}

	Bool Accelerators::dispatch(KeyChord chord) {
		Accel a = data->get(chord, Accel());
		if (!a.any())
			return false;

		a.call->call();
		return true;
	}

}
