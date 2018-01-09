// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef GUI_H
#define GUI_H

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

/**
 * For Gtk+, there are a few possible implementations of the rendering. They differ both in how
 * OpenGL interacts with the drawing in Gtk+.
 *
 * GTK_RENDER_SINGLE_GL - use one thread for OpenGL and Ui. Use OpenGL for updates, bypassing Gtk+.
 * GTK_RENDER_SINGLE_GL_COPY - use one thread for OpenGL and Ui. Copy the GL contents to cairo.
 *
 * TODO: Add multithreaded rendering somehow...
 */
#ifdef GUI_GTK

// #define GTK_RENDER_SINGLE_GL // Has issues with black backgrounds.
#define GTK_RENDER_SINGLE_GL_COPY

#if defined(GTK_RENDER_SINGLE_GL) || defined(GTK_RENDER_SINGLE_GL_COPY)
// Defined if both the Ui and Render thread are the same thread.
#define SINGLE_THREADED_UI
#endif

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
