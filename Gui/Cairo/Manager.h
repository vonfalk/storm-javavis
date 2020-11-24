#pragma once
#include "Gui/GraphicsMgr.h"
#include "Cairo.h"

namespace gui {

	STORM_PKG(impl);

	class CairoSurface;

	class CairoManager : public GraphicsMgrRaw {
		STORM_CLASS;
	public:
#ifdef GUI_GTK
		// Create.
		CairoManager(Graphics *owner, CairoSurface *surface);
#endif

		// Create resources:
		virtual void create(SolidBrush *brush, void *&result, Resource::Cleanup &cleanup);
		virtual void create(BitmapBrush *brush, void *&result, Resource::Cleanup &cleanup);
		virtual void create(LinearGradient *brush, void *&result, Resource::Cleanup &cleanup);
		virtual void create(RadialGradient *brush, void *&result, Resource::Cleanup &cleanup);
		virtual void create(Bitmap *bitmap, void *&result, Resource::Cleanup &cleanup);
		virtual void create(Path *path, void *&result, Resource::Cleanup &cleanup);

		// Update resources:
		virtual void update(SolidBrush *brush, void *resource);
		virtual void update(BitmapBrush *brush, void *resource);
		virtual void update(LinearGradient *brush, void *resource);
		virtual void update(RadialGradient *brush, void *resource);
		virtual void update(Bitmap *bitmap, void *resource);
		virtual void update(Path *path, void *result);

#ifdef GUI_GTK
		// Specific for this manager: apply a brush to a cairo context.
		static void applyBrush(cairo_t *to, Brush *brush, void *data);
#endif

	private:
		// Owner (to get bitmaps for bitmap brushes)
		Graphics *owner;

		// Surface.
		CairoSurface *surface;
	};

}
