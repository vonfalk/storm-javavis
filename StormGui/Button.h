#pragma once
#include "Container.h"

namespace stormgui {

	/**
	 * Button. TODO: Re-design the interface. Delay creation until the controls are actually added somewhere?
	 */
	class Button : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR Button(Par<Str> text, Par<Container> parent);
	};

}
