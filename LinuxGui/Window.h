#pragma once
#include "Handle.h"

namespace gui {

	/**
	 * Base class for windows and controls.
	 */
	class Window : public ObjectOn<Ui> {
		STORM_CLASS;
	public:
		STORM_CTOR Window();
		virtual ~Window();

		// Get/set handle.
		GtkWidget *handle() const;
		void handle(GtkWidget *widget);

		bool created() const { return handle() != null; }

		// Window text.
		const Str *STORM_FN text();
		void STORM_SETTER text(Str *str);

	protected:
		Handle myHandle;

		// Text.
		const Str *myText;
	};

}
