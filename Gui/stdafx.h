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
#else
#define GUI_GTK
#endif

#ifdef GUI_GTK
#include "GtkMode.h"

/**
 * For Gtk+, there are a few possible implementations of the rendering. They differ both in how
 * OpenGL interacts with the drawing in Gtk+.
 *
 * GTK_MT - multi-threaded
 * GTK_ST - single-threaded
 * GTK_MT_SW - multi-threaded, force software rendering
 * GTK_ST_SW - single-threaded, force software rendering
 *
 * Note: Client applications will notice when we are not multi threading rendering.
 */
#define GTK_MT (GTK_RENDER_MT)
#define GTK_ST (0)
#define GTK_MT_SW (GTK_RENDER_MT | GTK_RENDER_SW)
#define GTK_ST_SW (GTK_RENDER_SW)

// The selected mode. Pick one of the above.
#define GTK_MODE GTK_MT

#if GTK_RENDER_IS_MT(GTK_MODE)
#define UI_MULTITHREAD
#else
#define UI_SINGLETHREAD
#endif

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
