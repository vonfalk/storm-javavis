#pragma once
#include "Core/Hash.h"

namespace gui {

	/**
	 * Encapsulates a Win32 handle so that it can be properly understood by Storm.
	 */
	class Handle {
		STORM_VALUE;
	public:
		Handle() { value = 0; }

#ifdef GUI_WIN32
		Handle(HWND wnd) { value = (size_t)wnd; }
		Handle(ATOM atom) { value = (size_t)atom; }
		Handle(HINSTANCE instance) { value = (size_t)instance; }

		// Get Win32 types.
		inline HWND hwnd() const { return (HWND)value; }
		inline ATOM atom() const { return (ATOM)value; }
		inline HINSTANCE instance() const { return (HINSTANCE)value; }
#endif

#ifdef GUI_GTK
		Handle(GtkWidget *wnd) { value = (size_t)wnd; }

		inline GtkWidget *widget() const { return (GtkWidget *)value; }
#endif

		inline Bool STORM_FN operator ==(Handle o) const {
			return value == o.value;
		}

		inline Bool STORM_FN operator !=(Handle o) const {
			return value != o.value;
		}

		inline Nat STORM_FN hash() const {
			return ptrHash((void *)value);
		}

	private:
		UNKNOWN(PTR_NOGC) size_t value;
	};

}
