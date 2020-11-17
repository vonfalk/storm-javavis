#include "stdafx.h"
#include "Brush.h"
#include "Painter.h"
#include "Bitmap.h"
#include "GraphicsMgr.h"

namespace gui {

	Brush::Brush() {}

	SolidBrush::SolidBrush(Color c) : myOpacity(1.0f), myColor(c) {}

	void SolidBrush::create(GraphicsMgrRaw *g, void *&result, Cleanup &update) {
		g->create(this, result, update);
	}
	void SolidBrush::update(GraphicsMgrRaw *g, void *resource) {
		g->update(this, resource);
	}


#ifdef GUI_GTK

	void SolidBrush::prepare(cairo_t *cairo) {
		cairo_set_source_rgba(cairo, col.r, col.g, col.b, col.a * opacity);
	}

#endif

	BitmapBrush::BitmapBrush(Bitmap *bitmap) : myBitmap(bitmap), myTfm(new (engine()) Transform()) {}

	BitmapBrush::BitmapBrush(Bitmap *bitmap, Transform *tfm) : myBitmap(bitmap), myTfm(tfm) {}

	void BitmapBrush::transform(Transform *tfm) { /* dummy */ }

#ifdef GUI__WIN32

	void BitmapBrush::create(Painter *owner, ID2D1Resource **out) {
		D2D1_BITMAP_BRUSH_PROPERTIES props = {
			D2D1_EXTEND_MODE_WRAP,
			D2D1_EXTEND_MODE_WRAP,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
		};
		ID2D1BitmapBrush *brush = null;
		// owner->renderTarget()->CreateBitmapBrush(myBitmap->bitmap(owner), props, &brush);
		*out = brush;
		brush->SetTransform(dx(myTfm));
	}

	void BitmapBrush::transform(Transform *tfm) {
		myTfm = tfm;
		if (ID2D1BitmapBrush *b = peek<ID2D1BitmapBrush>()) {
			b->SetTransform(dx(myTfm));
		}
	}

#endif
#ifdef GUI_GTK

	OsResource *BitmapBrush::create(Painter *owner) {
		cairo_pattern_t *p = cairo_pattern_create_for_surface(myBitmap->get<cairo_surface_t>(owner));
		cairo_pattern_set_extend(p, CAIRO_EXTEND_REPEAT);

		cairo_matrix_t tfm = cairo(myTfm->inverted());
		cairo_pattern_set_matrix(p, &tfm);

		return p;
	}

	void BitmapBrush::transform(Transform *tfm) {
		myTfm = tfm;
		if (cairo_pattern_t *p = peek<cairo_pattern_t>()) {
			cairo_matrix_t tfm = cairo(myTfm->inverted());
			cairo_pattern_set_matrix(p, &tfm);
		}
	}

#endif

	GradientStop::GradientStop(Float position, Color color) : pos(position), color(color) {}

	wostream &operator <<(wostream &to, const GradientStop &s) {
		return to << s.color << L"@" << s.pos;
	}

	StrBuf &operator <<(StrBuf &to, GradientStop s) {
		return to << s.color << L"@" << s.pos;
	}


	Gradient::Gradient() : myStops(null) {}

	Gradient::Gradient(Array<GradientStop> *stops) : myStops(stops) {}

	Array<GradientStop> *Gradient::stops() const {
		return new (this) Array<GradientStop>(*myStops);
	}

	void Gradient::stops(Array<GradientStop> *stops) {
		myStops = new (this) Array<GradientStop>(*stops);

		// We need to re-create the backend-specific things.
		// Note: There is no destroy(), and if we had one, it would not work
		// since the refcount for shared items would break.
		// destroy();
		TODO(L"FIXME");
	}

#ifdef GUI_GTK

	void Gradient::applyStops(cairo_pattern_t *to) {
		for (Nat i = 0; i < myStops->count(); i++) {
			GradientStop &at = myStops->at(i);
			cairo_pattern_add_color_stop_rgba(to, at.pos, at.color.r, at.color.g, at.color.b, at.color.a);
		}
	}

#endif

	LinearGradient::LinearGradient(Array<GradientStop> *stops, Point start, Point end) :
		Gradient(stops), myStart(start), myEnd(end) {}

	LinearGradient::LinearGradient(Color c1, Color c2, Point start, Point end) :
		Gradient(), myStart(start), myEnd(end) {

		Array<GradientStop> *s = new (this) Array<GradientStop>();
		s->push(GradientStop(0, c1));
		s->push(GradientStop(1, c2));
		stops(s);
	}

	void LinearGradient::start(Point p) {
		myStart = p;
		needUpdate();
	}

	void LinearGradient::end(Point p) {
		myEnd = p;
		needUpdate();
	}

	void LinearGradient::points(Point start, Point end) {
		myStart = start;
		myEnd = end;
		needUpdate();
	}

	void LinearGradient::create(GraphicsMgrRaw *g, void *&result, Cleanup &update) {
		g->create(this, result, update);
	}
	void LinearGradient::update(GraphicsMgrRaw *g, void *resource) {
		g->update(this, resource);
	}

#ifdef GUI_GTK

	OsResource *LinearGradient::create(Painter *owner) {
		// We're using the points (0, 0) - (0, -1) so that we can easily transform them later (= 0 deg).
		cairo_pattern_t *r = cairo_pattern_create_linear(0, 0, 0, -1);
		applyStops(r);
		updatePoints(r);
		return r;
	}

	void LinearGradient::updatePoints() {
		if (cairo_pattern_t *p = peek<cairo_pattern_t>()) {
			updatePoints(p);
		}
	}

	void LinearGradient::updatePoints(cairo_pattern_t *p) {
		Point delta = myEnd - myStart;
		Angle a = angle(delta);
		Float scale = 1.0f / delta.length();

		// Compute a matrix so that 'start' and 'end' map to the points (0, 0) and (0, 1).
		cairo_matrix_t tfm;
		// Note: transformations are applied in reverse.
		cairo_matrix_init_rotate(&tfm, -a.rad());
		cairo_matrix_scale(&tfm, scale, scale);
		cairo_matrix_translate(&tfm, -myStart.x, -myStart.y);
		cairo_pattern_set_matrix(p, &tfm);
	}

#endif

	RadialGradient::RadialGradient(Array<GradientStop> *stops, Point center, Float radius) :
		Gradient(stops), myCenter(center), myRadius(radius), myTransform(new (engine()) Transform()) {}

	RadialGradient::RadialGradient(Color c1, Color c2, Point center, Float radius) :
		Gradient(), myCenter(center), myRadius(radius), myTransform(new (engine()) Transform()) {

		Array<GradientStop> *s = new (this) Array<GradientStop>();
		s->push(GradientStop(0, c1));
		s->push(GradientStop(1, c2));
		stops(s);
	}

	void RadialGradient::center(Point p) {
		myCenter = p;
		needUpdate();
	}

	void RadialGradient::radius(Float v) {
		myRadius = v;
		needUpdate();
	}

	void RadialGradient::transform(Transform *tfm) {
		myTransform = tfm;
		needUpdate();
	}

	void RadialGradient::create(GraphicsMgrRaw *g, void *&result, Cleanup &update) {
		g->create(this, result, update);
	}
	void RadialGradient::update(GraphicsMgrRaw *g, void *resource) {
		g->update(this, resource);
	}

#ifdef GUI_GTK

	OsResource *RadialGradient::create(Painter *owner) {
		// We're using the point (0, 0) and a radius of 1 so that we can easily transform it later.
		cairo_pattern_t *r = cairo_pattern_create_radial(0, 0, 0, 0, 0, 1);
		applyStops(r);
		update(r);
		return r;
	}

	void RadialGradient::update() {
		if (cairo_pattern_t *p = peek<cairo_pattern_t>()) {
			update(p);
		}
	}

	void RadialGradient::update(cairo_pattern_t *p) {
		Float r = 1.0f / myRadius;

		cairo_matrix_t tfm;
		cairo_matrix_init_scale(&tfm, r, r);
		cairo_matrix_translate(&tfm, -myCenter.x, -myCenter.y);

		cairo_matrix_t our = cairo(myTransform->inverted());
		cairo_matrix_multiply(&tfm, &our, &tfm);

		cairo_pattern_set_matrix(p, &tfm);
	}

#endif

}
