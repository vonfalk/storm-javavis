#pragma once
#include "Core/TObject.h"

namespace gui {

	/**
	 * Resource used in DX somewhere.
	 */
	class Resource : public ObjectOn<Ui> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Resource();

		// Destroy.
		~Resource();

		// Destroy the resource.
		virtual void destroy();
	};

}
