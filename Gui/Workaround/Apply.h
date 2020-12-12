#pragma once
#include "Gui/Env.h"
#include "Gui/Device.h"
#include "Gui/Surface.h"

namespace gui {

	/**
	 * Interface to pick suitable workarounds.
	 */

	// Apply workarounds to a device based on environment variables.
	Device *applyEnvWorkarounds(Device *device);

#ifdef GUI_GTK

	// Apply workarounds required for a particular OpenGL context. Assumes that the OpenGL context
	// in question has been made current. Will not do anything in case the environment variable is
	// set.
	Surface *applyGLWorkarounds(Surface *surface);

#endif

}
