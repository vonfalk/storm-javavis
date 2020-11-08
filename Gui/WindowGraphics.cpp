#include "stdafx.h"
#include "WindowGraphics.h"
#include "Painter.h"
#include "Brush.h"
#include "Text.h"
#include "Path.h"
#include "Bitmap.h"

namespace gui {

	WindowGraphics::WindowGraphics() {}

	void WindowGraphics::surfaceResized() {}

	void WindowGraphics::surfaceDestroyed() {}

	void WindowGraphics::beforeRender(Color) {}

	bool WindowGraphics::afterRender() { return true; }

}
