// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef LINUX_GUI_H
#define LINUX_GUI_H
#include "Shared/Storm.h"
#include "Core/TObject.h"
#include "Core/EnginePtr.h"
#include "Core/Graphics/Color.h"
#include "Core/Geometry/Angle.h"
#include "Core/Geometry/Point.h"
#include "Core/Geometry/Rect.h"
#include "Core/Geometry/Size.h"
#include "Core/Geometry/Transform.h"
#include "Core/Geometry/Vector.h"

#include <gtk/gtk.h>

namespace gui {
	using namespace storm;
	using namespace storm::geometry;

	STORM_THREAD(Ui);
	STORM_THREAD(Render);
}

#endif
