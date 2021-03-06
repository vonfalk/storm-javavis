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
 * GTK_ST_HW - single-threaded, hardware acceleration, swap buffers whenever ready
 * GTK_ST_HW_SYNC - single-threaded, hardware acceleration, sync buffer swap with Gtk+
 * GTK_ST_HW_COPY - single-threaded, hardware acceleration, render to separate texture, and copy in sync with Gtk+
 * GTK_ST_HW_CAIRO - single-threaded, hardware acceleration, copy to Cairo
 * GTK_MT_HW - multi-threaded, hardware acceleration, swap buffers whenever ready
 * GTK_MT_HW_SYNC - multi-threaded, hardware acceleration, sync buffer swap with Gtk+
 * GTK_ST_HW_COPY - multi-threaded, hardware acceleration, render to separate texture, and copy in sync with Gtk+
 * GTK_MT_HW_CAIRO - multi-threaded, hardware acceleration, copy to Cairo
 * GTK_ST_SW - single-threaded, software rendering
 * GTK_MT_SW - multi-threaded, software rendering
 *
 * Note: Client applications will notice when we are not multi threading rendering.
 *
 * GTK_??_HW and GTK_??_HW_SYNC have the issue that parts of the rendering will not be present
 * in the output. Usually the background layers will be missing.
 *
 * GTK_??_HW_COPY and GTK_??_HW_CAIRO work well. _COPY should be slightly faster on most systems,
 * but may produce erroneous results from time to time. _COPY does not currently work with Wayland
 * while _CAIRO does.
 */
#define GTK_ST_HW (GTK_RENDER_SWAP)
#define GTK_ST_HW_SYNC (GTK_RENDER_SYNC)
#define GTK_ST_HW_COPY (GTK_RENDER_COPY)
#define GTK_ST_HW_CAIRO (GTK_RENDER_CAIRO)
#define GTK_MT_HW (GTK_RENDER_MT | GTK_RENDER_SWAP)
#define GTK_MT_HW_SYNC (GTK_RENDER_MT | GTK_RENDER_SYNC)
#define GTK_MT_HW_COPY (GTK_RENDER_MT | GTK_RENDER_COPY)
#define GTK_MT_HW_CAIRO (GTK_RENDER_MT | GTK_RENDER_CAIRO)
#define GTK_ST_SW (GTK_RENDER_SW | GTK_RENDER_CAIRO)
#define GTK_MT_SW (GTK_RENDER_SW | GTK_RENDER_CAIRO | GTK_RENDER_MT)

// The selected mode. Pick one of the above.
// Note: GTK_MT_HW_COPY have stuttering issues on some systems. Repaints seems to be done
// properly, but the image on the screen is not always updated. I think this is due to us
// fighting with Gtk+, as everything seems to work better with the GTK_MT_HW_CAIRO (even
// though there are more VSync artifacts in that case).
// Note: GTK_*_CAIRO are currently the only modes that are supported by Wayland.
#define GTK_MODE GTK_MT_HW_CAIRO


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
