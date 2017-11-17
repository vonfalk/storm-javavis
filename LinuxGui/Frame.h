#pragma once
#include "Container.h"

namespace gui {

	class Frame : public Container {
		STORM_CLASS;
	public:
		STORM_CTOR Frame(Str *title);

		// Create it.
		void STORM_FN create();

		// Close this frame.
		virtual void STORM_FN close();
	};

}
