// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef GUI_H
#define GUI_H

#include "Utils/Platform.h"

/**
 * Determine which Gui toolkit to use based on the platform.
 *
 * Supported:
 * GUI_WIN32 - Win32 (native)
 * GUI_GTK   - Gtk+3.0
 */

#ifdef WINDOWS
#define GUI_WIN32
#else
#define GUI_GTK
#endif


// Allow inclusion from C as well.
#ifdef __cplusplus

#include "Shared/Storm.h"
#include "Core/Graphics/Color.h"
#include "Core/Geometry/Angle.h"
#include "Core/Geometry/Point.h"
#include "Core/Geometry/Rect.h"
#include "Core/Geometry/Size.h"
#include "Core/Geometry/Transform.h"
#include "Core/Geometry/Vector.h"

/**
 * Common declarations.
 */
namespace gui {
	using namespace storm;
	using namespace storm::geometry;

	/**
	 * Thread used for interaction with the Ui library.
	 *
	 * All event handling and rendering is done on this thread.
	 *
	 * Note: We previously had a separate Render thread. It turns out that this is a bit overkill in
	 * most situations, as input is inherently tied to rendering, and separating the two causes
	 * unnecessary overhead (which is fine by itself). It also hinders hooking into the regular
	 * "drawing pipeline" in a nice way (e.g. no customization of the look of widgets). Finally, it
	 * turns out that windowing systems on Linux is really not designed for background rendering
	 * (e.g. threading support in Pango seems dubious, even if it is supposedly safe now), which
	 * causes many headaches on Linux.
	 */
	STORM_THREAD(Ui);

}


/**
 * Include more specific things based on the current UI library:
 */
#include "Win32Common.h"
#include "GtkCommon.h"

#endif
#endif
