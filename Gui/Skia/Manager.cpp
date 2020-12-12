#include "stdafx.h"
#include "Manager.h"
#include "Skia.h"

namespace gui {

	SkiaManager::SkiaManager() {}

#ifdef GUI_GTK

	// Everything in here is a "SkPaint"
	static void skiaCleanup(void *param) {
		SkPaint *paint = (SkPaint *)param;
		delete paint;
	}

	void SkiaManager::create(SolidBrush *brush, void *&result, Resource::Cleanup &cleanup) {
		Color c = brush->color();
		c.a *= brush->opacity();
		SkPaint *paint = new SkPaint(skia(c));
		paint->setAntiAlias(true);

		cleanup = &skiaCleanup;
		result = paint;
	}

	void SkiaManager::update(SolidBrush *brush, void *resource) {
		SkPaint *paint = (SkPaint *)resource;

		Color c = brush->color();
		c.a *= brush->opacity();
		paint->setColor(skia(c));
	}

	void SkiaManager::create(BitmapBrush *brush, void *&result, Resource::Cleanup &cleanup) {
		TODO(L"Implement bitmap brushes for Skia!");

		result = null;
		cleanup = null;
	}

	void SkiaManager::update(BitmapBrush *brush, void *resource) {}

	void SkiaManager::create(LinearGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		// It seems like we can utilize the matrix in the shader with some further digging...

		SkPaint *paint = new SkPaint();
		paint->setAntiAlias(true);

		Array<GradientStop> *stops = brush->peekStops();
		SkPoint points[2] = {
			skia(brush->start()), skia(brush->end())
		};
		SkColor *colors = (SkColor *)alloca(stops->count() * sizeof(SkScalar));
		SkScalar *pos = (SkScalar *)alloca(stops->count() * sizeof(SkScalar));
		for (Nat i = 0; i < stops->count(); i++) {
			colors[i] = skia(stops->at(i).color).toSkColor();
			pos[i] = stops->at(i).pos;
		}
		paint->setShader(SkGradientShader::MakeLinear(points, colors, pos, stops->count(), SkTileMode::kClamp));

		result = paint;
		cleanup = &skiaCleanup;
	}

	void SkiaManager::update(LinearGradient *brush, void *resource) {
		TODO(L"Implement the update function!");
	}

	void SkiaManager::create(RadialGradient *brush, void *&result, Resource::Cleanup &cleanup) {}

	void SkiaManager::update(RadialGradient *brush, void *resource) {}

	void SkiaManager::create(Bitmap *bitmap, void *&result, Resource::Cleanup &cleanup) {}

	void SkiaManager::update(Bitmap *, void *) {}

	void SkiaManager::create(Path *path, void *&result, Resource::Cleanup &cleanup) {}

	void SkiaManager::update(Path *, void *) {}

#else

	DEFINE_GRAPHICS_MGR_FNS(SkiaManager);

#endif

}
