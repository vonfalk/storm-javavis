#pragma once
#include "Font.h"

namespace gui {

	/**
	 * Get default values for the look and feel of the current system.
	 *
	 * Note: Do not attempt to allocate this struct on the heap. It contains GC allocated objects
	 * that are not properly scanned if the struct is heap allocated.
	 */
	struct Defaults {
		// Default font
		Font *font;

		// Default window background color.
		Color bgColor;

		// Default text color.
		Color textColor;
	};

	// Get defaults.
	Defaults sysDefaults(EnginePtr e);

}
