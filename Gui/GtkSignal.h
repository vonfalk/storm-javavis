#pragma once
#include "App.h"

#ifdef GUI_GTK

namespace gui {

	/**
	 * Wrapper around signals in Gtk so that they can get delivered to the appropriate Windows.
	 */

	template <class Result, class Class, class ... Params>
	struct Signal {
		template <Result (Class::*fn)(Params...)>
		struct Connect {
			static void to(GtkWidget *to, const gchar *name, Engine &e) {
				g_signal_connect(to, name, (GCallback)&callback, &e);
			}

			static Result callback(GtkWidget *src, Params... args, gpointer engine) {
				Engine *e = (Engine *)engine;
				App *app = gui::app(*e);
				Window *win = app->findWindow(Handle(src));
				if (!win) {
					WARNING(L"Unknown window!");
					return Result();
				}
				Class *c = as<Class>(win);
				if (!c) {
					WARNING(L"Invalid subclass. Got " << win);
					return Result();
				}

				return (c->*fn)(args...);
			}
		};
	};

}

#endif