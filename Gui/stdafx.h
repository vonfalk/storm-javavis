// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef GUI_H
#define GUI_H

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
 * GTK_ST_HW - single-threaded, hardware acceleration
 * GTK_ST_HW_CAIRO - single-threaded, hardware acceleration, copy to Cairo
 * GTK_MT_HW - multi-threaded, hardware acceleration
 * GTK_MT_HW_CAIRO - multi-threaded, hardware acceleration, copy to Cairo
 * GTK_ST_SW - single-threaded, software rendering
 * GTK_MT_SW - multi-threaded, software rendering
 *
 * Note: Client applications will notice when we are not multi threading rendering.
 */
#define GTK_ST_HW (0)
#define GTK_ST_HW_CAIRO (GTK_RENDER_CAIRO)
#define GTK_MT_HW (GTK_RENDER_MT)
#define GTK_MT_HW_CAIRO (GTK_RENDER_MT | GTK_RENDER_CAIRO)
#define GTK_ST_SW (GTK_RENDER_SW | GTK_RENDER_CAIRO)
#define GTK_MT_SW (GTK_RENDER_SW | GTK_RENDER_CAIRO | GTK_RENDER_MT)

// The selected mode. Pick one of the above.
#define GTK_MODE GTK_MT_SW


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

	STORM_THREAD(Ui);
	STORM_THREAD(Render);

}


/**
 * Include more specific things based on the current UI library:
 */
#include "Win32Common.h"
#include "GtkCommon.h"

#endif
#endif
