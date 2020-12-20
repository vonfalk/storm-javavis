#include "stdafx.h"
#include "Manager.h"
#include "Skia.h"
#include "LocalShader.h"

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

	static SkMatrix linearMatrix(Point start, Point end) {
		Point delta = end - start;
		Angle a = angle(delta);
		Float scale = delta.length();

		SkMatrix tfm = SkMatrix::RotateRad(a.rad());
		tfm.postScale(scale, scale);
		tfm.postTranslate(start.x, start.y);
		return tfm;
	}

	void SkiaManager::create(LinearGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		SkPaint *paint = new SkPaint();
		paint->setAntiAlias(true);

		Array<GradientStop> *stops = brush->peekStops();
		// We transform these points later. Using (0, 0) - (0, -1) makes it easy to compute this transform.
		SkPoint points[2] = {
			{ 0, 0 },
			{ 0, -1 },
		};
		SkColor *colors = (SkColor *)alloca(stops->count() * sizeof(SkColor));
		SkScalar *pos = (SkScalar *)alloca(stops->count() * sizeof(SkScalar));
		for (Nat i = 0; i < stops->count(); i++) {
			colors[i] = skia(stops->at(i).color).toSkColor();
			pos[i] = stops->at(i).pos;
		}
		sk_sp<SkShader> shader = SkGradientShader::MakeLinear(points, colors, pos, stops->count(), SkTileMode::kClamp);
		shader = sk_make_sp<LocalShader>(shader, linearMatrix(brush->start(), brush->end()));
		paint->setShader(shader);

		result = paint;
		cleanup = &skiaCleanup;
	}

	void SkiaManager::update(LinearGradient *brush, void *resource) {
		SkPaint *paint = (SkPaint *)resource;
		SkShader *shader = paint->getShader();
		// We always use a LocalShader in between.
		LocalShader *local = (LocalShader *)shader;
		local->matrix = linearMatrix(brush->start(), brush->end());
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
