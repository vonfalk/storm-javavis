// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef GUI_H
#define GUI_H

#include "Shared/Storm.h"
#include "Core/Graphics/Color.h"
#include "Core/Geometry/Angle.h"
#include "Core/Geometry/Point.h"
#include "Core/Geometry/Rect.h"
#include "Core/Geometry/Size.h"
#include "Core/Geometry/Transform.h"
#include "Core/Geometry/Vector.h"

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
