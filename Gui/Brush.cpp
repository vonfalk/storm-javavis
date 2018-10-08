#include "stdafx.h"
#include "Brush.h"
#include "Painter.h"
#include "Bitmap.h"

namespace gui {

	Brush::Brush() {}

#ifdef GUI_WIN32

	void Brush::prepare(ID2D1Brush *b) {}

#endif
#ifdef GUI_GTK

	void Brush::prepare(const Rect &r, cairo_pattern_t *b) {}
	void Brush::prepare(const Rect &r, cairo_t *b) {}

#endif

	SolidBrush::SolidBrush(Color c) : opacity(1.0f), col(c) {}

#ifdef GUI_WIN32

	void SolidBrush::create(Painter *owner, ID2D1Resource **out) {
		owner->renderTarget()->CreateSolidColorBrush(dx(col), (ID2D1SolidColorBrush **)out);
	}

	void SolidBrush::prepare(ID2D1Brush *b) {
		b->SetOpacity(opacity);
	}

#endif
#ifdef GUI_GTK

	OsResource *SolidBrush::create(Painter *owner) {
		// return cairo_pattern_create_rgba(col.r, col.g, col.b, col.a);
		return 0;
	}

	void SolidBrush::prepare(const Rect &bound, cairo_t *cairo) {
		cairo_set_source_rgba(cairo, col.r, col.g, col.b, col.a * opacity);
	}

#endif

	BitmapBrush::BitmapBrush(Bitmap *bitmap) : myBitmap(bitmap), myTfm(new (engine()) Transform()) {}

	BitmapBrush::BitmapBrush(Bitmap *bitmap, Transform *tfm) : myBitmap(bitmap), myTfm(tfm) {}

#ifdef GUI_WIN32

	void BitmapBrush::create(Painter *owner, ID2D1Resource **out) {
		D2D1_BITMAP_BRUSH_PROPERTIES props = {
			D2D1_EXTEND_MODE_WRAP,
			D2D1_EXTEND_MODE_WRAP,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
		};
		ID2D1BitmapBrush *brush = null;
		owner->renderTarget()->CreateBitmapBrush(myBitmap->bitmap(owner), props, &brush);
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
		cairo_pattern_t *p = cairo_pattern_create_for_surface(bitmap->get<cairo_surface_t>(owner));
		cairo_pattern_set_extend(p, CAIRO_EXTEND_REPEAT);
		return p;
	}

	void BitmapBrush::prepare(const Rect &bound, cairo_pattern_t *brush) {
		cairo_matrix_t tfm;
		cairo_matrix_init_translate(&tfm, -bound.p0.x, -bound.p0.y);
		cairo_pattern_set_matrix(brush, &tfm);
	}

#endif

	GradientStop::GradientStop(Float position, Color color) : pos(position), color(color) {}

	wostream &operator <<(wostream &to, const GradientStop &s) {
		return to << s.color << L"@" << s.pos;
	}

	StrBuf &operator <<(StrBuf &to, GradientStop s) {
		return to << s.color << L"@" << s.pos;
	}


	Gradient::Gradient() : dxObject(null), myStops(null) {}

	Gradient::Gradient(Array<GradientStop> *stops) : dxObject(null), myStops(stops) {}

	Gradient::~Gradient() {
		destroy();
	}

	void Gradient::destroy() {
		Brush::destroy();
		::release(dxObject);
	}

	Array<GradientStop> *Gradient::stops() const {
		return new (this) Array<GradientStop>(*myStops);
	}

	void Gradient::stops(Array<GradientStop> *stops) {
		myStops = new (this) Array<GradientStop>(*stops);
		destroy();
	}

#ifdef GUI_WIN32

	ID2D1GradientStopCollection *Gradient::dxStops(Painter *owner) {
		if (!dxObject) {
			D2D1_GRADIENT_STOP *tmp = (D2D1_GRADIENT_STOP *)alloca(sizeof(D2D1_GRADIENT_STOP) * myStops->count());
			for (Nat i = 0; i < myStops->count(); i++) {
				tmp[i].position = myStops->at(i).pos;
				tmp[i].color = dx(myStops->at(i).color);
			}

			owner->renderTarget()->CreateGradientStopCollection(tmp, myStops->count(), &dxObject);
		}
		return dxObject;
	}

#endif
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

	LinearGradient::LinearGradient(Color a, Color b, Point start, Point end) :
		Gradient(), myStart(start), myEnd(end) {

		Array<GradientStop> *s = CREATE(Array<GradientStop>, this);
		s->push(GradientStop(0, a));
		s->push(GradientStop(1, b));
		stops(s);
	}

	void LinearGradient::start(Point p) {
		myStart = p;
		updatePoints();
	}

	void LinearGradient::end(Point p) {
		myEnd = p;
		updatePoints();
	}

	void LinearGradient::points(Point start, Point end) {
		myStart = start;
		myEnd = end;
		updatePoints();
	}

#ifdef GUI_WIN32

	void LinearGradient::create(Painter *owner, ID2D1Resource **out) {
		D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES p = { dx(myStart), dx(myEnd) };
		owner->renderTarget()->CreateLinearGradientBrush(p, dxStops(owner), (ID2D1LinearGradientBrush**)out);
	}

	void LinearGradient::updatePoints() {
		if (ID2D1LinearGradientBrush *o = peek<ID2D1LinearGradientBrush>()) {
			o->SetStartPoint(dx(myStart));
			o->SetEndPoint(dx(myEnd));
		}
	}

#endif
#ifdef GUI_GTK

	OsResource *LinearGradient::create(Painter *owner) {
		// Using the points (0, 1) - (0, -1) (= 0 deg) for simplicity. We'll transform it later.
		cairo_pattern_t *r = cairo_pattern_create_linear(0, 1, 0, -1);
		applyStops(r);
		return r;
	}

	void LinearGradient::prepare(const Rect &bound, cairo_pattern_t *r) {
		Size size = bound.size();
		Point center = bound.center();

		float w = max(size.w, size.h);
		float scale = 1.0f / w;

		// Compute a matrix so that 'start' and 'end' map to the points (0, 0) and (1, 0).
		cairo_matrix_t tfm;
		// Note: transformations are applied in reverse.
		cairo_matrix_init_rotate(&tfm, -angle.rad());
		cairo_matrix_scale(&tfm, scale, scale);
		cairo_matrix_translate(&tfm, -center.x, -center.y);
		cairo_pattern_set_matrix(r, &tfm);
	}

#endif

}
