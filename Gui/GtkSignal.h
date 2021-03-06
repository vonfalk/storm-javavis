#pragma once
#include "App.h"
#include "Core/Exception.h"

#ifdef GUI_GTK

namespace gui {

	/**
	 * Wrapper around signals in Gtk so that they can get delivered to the appropriate Windows.
	 */

	template <class Result, class Class, class ... Params>
	struct Signal {
		template <Result (Class::*fn)(Params...)>
		struct Connect {
			// Connect to a signal associated with a Window.
			static void to(GtkWidget *to, const gchar *name, Engine &e) {
				g_signal_connect(to, name, (GCallback)&callback, &e);
			}

			static void last(GtkWidget *to, const gchar *name, Engine &e) {
				g_signal_connect_after(to, name, (GCallback)&callback, &e);
			}


			static Result callback(GtkWidget *src, Params... args, gpointer engine) {
				Engine *e = (Engine *)engine;
				App *app = gui::app(*e);

				GtkWidget *at = src;
				Window *win = null;
				while ((win = app->findWindow(Handle(at))) == null) {
					at = gtk_widget_get_parent(at);
					if (!at) {
						WARNING(L"Unknown window: " << src);
						return Result();
					}
				}

				Class *c = as<Class>(win);
				if (!c) {
					WARNING(L"Invalid subclass. Got " << win);
					return Result();
				}

				// Note: We need to catch exceptions since we can not throw them through Gtk+ code.
				try {
					return (c->*fn)(args...);
				} catch (const storm::Exception *e) {
					PLN(L"Unhandled exception in window thread:\n" << e->message());
				} catch (const ::Exception &e) {
					PLN(L"Unhandled exception in window thread:\n" << e);
				} catch (...) {
					PLN(L"Unhandled exception in window thread: <unknown>");
				}
				return Result();
			}
		};
	};

}

#endif
