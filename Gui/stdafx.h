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
 *
 * Depending on the platform, one of the following flags will also be set:
 * UI_MULTITHREAD
 * UI_SINGLETHREAD
 */

#ifdef WINDOWS

#define GUI_WIN32
#define UI_MULTITHREAD

// Disable the GTK backends on win32 in case they were accidentally enabled.
#ifdef GUI_ENABLE_SKIA
#undef GUI_ENABLE_SKIA
#endif

#ifdef GUI_ENABLE_CAIRO_GL
#undef GUI_ENABLE_CAIRO_GL
#endif

#else

#define GUI_GTK

// Define either GUI_MULTITHREAD or GUI_SINGLETHREAD. Note: using a single-threaded UI will be
// visible to users of the API.
#define UI_MULTITHREAD
// #define UI_SINGLETHREAD

#endif



#if defined(UI_MULTITHREAD) == defined(UI_SINGLETHREAD)
#error "Pick either single- or multithreaded Ui!"
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
	 * Thread used for interaction with the Ui library. All events regarding windows are handled by
	 * this thread.
	 */
	STORM_THREAD(Ui);

	/**
	 * Thread used for rendering. This thread is dedicated to drawing custom contents inside the
	 * windows so that events from the operating system can be handled within reasonable time even
	 * when heavy rendering work is being done.
	 */
	STORM_THREAD(Render);

}


/**
 * Include more specific things based on the current UI library:
 */
#include "Win32Common.h"
#include "GtkCommon.h"

#endif
#endif
