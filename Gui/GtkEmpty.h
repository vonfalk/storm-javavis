#pragma once

#ifdef GUI_GTK

/**
 * Empty widget created by Window if not overloaded. GtkDrawingArea creates its own window, which is
 * unsuitable for our needs.
 */

#define TYPE_EMPTY empty_get_type()
#define EMPTY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_EMPTY, Empty))
#define EMPTY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), TYPE_EMPTY, EmptyClass))
#define IS_EMPTY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_EMPTY))

typedef struct _Empty Empty;
typedef struct _EmptyClass EmptyClass;

struct _Empty {
	GtkWidget container;
};

struct _EmptyClass {
	GtkWidgetClass parent;
};


// Get the type.
GType empty_get_type();

// Create a new layout instance.
GtkWidget *empty_new();

#endif
