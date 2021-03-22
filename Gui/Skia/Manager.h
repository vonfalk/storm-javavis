#pragma once
#include "Gui/GraphicsMgr.h"
#include "Skia.h"

namespace gui {

	STORM_PKG(impl);

	class SkiaSurface;

	class SkiaManager : public GraphicsMgrRaw {
		STORM_CLASS;
	public:
		// Create.
		SkiaManager(Graphics *owner, SkiaSurface &surface);

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

	private:
		// Owner. To get bitmap brushes.
		Graphics *owner;

		// Surface.
		SkiaSurface *surface;
	};

#ifdef GUI_ENABLE_SKIA

	/**
	 * Simple representation of bitmaps/images.
	 */
	struct SkiaBitmap {
		// The source image itself.
		sk_sp<SkImage> image;

		// Shader created for using it in a brush. Might be null.
		sk_sp<SkShader> brushShader;
	};

#endif

}
