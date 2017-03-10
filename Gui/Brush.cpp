#include "stdafx.h"
#include "Brush.h"
#include "Painter.h"
#include "Bitmap.h"

namespace gui {

	Brush::Brush() : opacity(1.0f) {}

	void Brush::prepare(const Rect &r, ID2D1Brush *b) {}

	SolidBrush::SolidBrush(Color c) : color(c) {}

	void SolidBrush::create(Painter *owner, ID2D1Resource **out) {
		owner->renderTarget()->CreateSolidColorBrush(dx(color), (ID2D1SolidColorBrush **)out);
	}

	BitmapBrush::BitmapBrush(Bitmap *bitmap) : bitmap(bitmap) {}

	void BitmapBrush::create(Painter *owner, ID2D1Resource **out) {
		D2D1_BITMAP_BRUSH_PROPERTIES props = {
			D2D1_EXTEND_MODE_WRAP,
			D2D1_EXTEND_MODE_WRAP,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
		};
		owner->renderTarget()->CreateBitmapBrush(bitmap->bitmap(owner), props, (ID2D1BitmapBrush **)out);
	}

	void BitmapBrush::prepare(const Rect &bound, ID2D1Brush *r) {
		r->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(bound.p0.x, bound.p0.y)));
	}

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
		::release(dxObject);
	}

	void Gradient::destroy() {
		Brush::destroy();
		::release(dxObject);
	}

	ID2D1GradientStopCollection *Gradient::dxStops(Painter *owner) {
		if (!dxObject) {
			D2D1_GRADIENT_STOP *tmp = (D2D1_GRADIENT_STOP *)alloca(sizeof(D2D1_GRADIENT_STOP) * myStops->count());
			for (nat i = 0; i < myStops->count(); i++) {
				tmp[i].position = myStops->at(i).pos;
				tmp[i].color = dx(myStops->at(i).color);
			}

			owner->renderTarget()->CreateGradientStopCollection(tmp, myStops->count(), &dxObject);
		}
		return dxObject;
	}

	void Gradient::stops(Array<GradientStop> *stops) {
		myStops = new (this) Array<GradientStop>(*stops);
		destroy();
	}

	LinearGradient::LinearGradient(Array<GradientStop> *stops, Angle angle) : Gradient(stops), angle(angle) {}

	LinearGradient::LinearGradient(Color a, Color b, Angle angle) : Gradient(), angle(angle) {
		Array<GradientStop> *s = CREATE(Array<GradientStop>, this);
		s->push(GradientStop(0, a));
		s->push(GradientStop(1, b));
		stops(s);
	}

	void LinearGradient::create(Painter *owner, ID2D1Resource **out) {
		Point start(0, 0), end(1, 1);
		D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES p = { dx(start), dx(end) };
		owner->renderTarget()->CreateLinearGradientBrush(p, dxStops(owner), (ID2D1LinearGradientBrush**)out);
	}

	void LinearGradient::prepare(const Rect &rect, ID2D1Brush *r) {
		Point start, end;
		compute(rect, start, end);

		ID2D1LinearGradientBrush *o = (ID2D1LinearGradientBrush *)r;
		o->SetStartPoint(dx(start));
		o->SetEndPoint(dx(end));
	}

	void LinearGradient::compute(const Rect &bound, Point &start, Point &end) {
		Size size = bound.size();
		Point dir = storm::geometry::angle(angle);
		Point c = center(size);

		float w = max(size.w, size.h);
		start = c + dir * -w;
		end = c + dir * w;

		start += bound.p0;
		end += bound.p0;
	}

}
