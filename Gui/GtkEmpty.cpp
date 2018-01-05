#include "stdafx.h"
#include "GtkEmpty.h"

#ifdef GUI_GTK

G_DEFINE_TYPE(Empty, empty, GTK_TYPE_WIDGET)

static void empty_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
	gtk_widget_set_allocation(widget, allocation);
}

GtkWidget *empty_new() {
	return (GtkWidget *)g_object_new(TYPE_EMPTY, NULL);
}

static void empty_init(Empty *instance) {
	gtk_widget_set_has_window(GTK_WIDGET(instance), false);
}

static void empty_class_init(EmptyClass *klass) {
	GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;

	widget_class->size_allocate = &empty_size_allocate;
}

#endif
