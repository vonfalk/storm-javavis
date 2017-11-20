#pragma once

#ifdef GUI_GTK

/**
 * Custom layout for Gtk+. A lot like 'fixed' but allows setting the width and height of child
 * widgets as well as their position.
 */

#define TYPE_BASIC basic_get_type()
#define BASIC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_BASIC, Basic))
#define BASIC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), TYPE_BASIC, BasicClass))
#define IS_BASIC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_BASIC))

typedef struct _Basic Basic;
typedef struct _BasicClass BasicClass;
typedef struct _BasicChild BasicChild;

struct _Basic {
	GtkContainer container;

	GList *children;
};

struct _BasicClass {
	GtkContainerClass parent;

	// More members?
};

struct _BasicChild {
	GtkWidget *widget;
	gint x;
	gint y;
	gint w;
	gint h;
};

// Get the type.
GType basic_get_type();

// Create a new layout instance.
GtkWidget *basic_new();

// Add a child widget to the layout.
void basic_put(Basic *layout, GtkWidget *widget, gint x, gint y, gint w, gint h);

// Move a child widget.
void basic_move(Basic *layout, GtkWidget *widget, gint x, gint y, gint w, gint h);

#endif
