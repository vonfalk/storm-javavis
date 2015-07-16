#include "stdafx.h"
#include "Brush.h"
#include "Painter.h"

namespace stormgui {

	void Brush::prepare(const Rect &r) {}

	SolidBrush::SolidBrush(Color c) : color(c) {}

	void SolidBrush::create(Painter *owner, ID2D1Resource **out) {
		owner->renderTarget()->CreateSolidColorBrush(dx(color), (ID2D1SolidColorBrush **)out);
	}

	GradientStop::GradientStop(Float position, Color color) : pos(position), color(color) {}

	wostream &operator <<(wostream &to, const GradientStop &s) {
		return to << s.color << "@" << s.pos;
	}

	Str *toS(EnginePtr e, GradientStop s) {
		return CREATE(Str, e.v, ::toS(s));
	}


	Gradient::Gradient(Par<Array<GradientStop>> stops) : dxObject(null) {
		this->stops(stops);
	}

	Gradient::~Gradient() {
		::release(dxObject);
	}

	void Gradient::destroy() {
		::release(dxObject);
	}

	ID2D1GradientStopCollection *Gradient::dxStops(Painter *owner) {
		if (!dxObject)
			owner->renderTarget()->CreateGradientStopCollection(&myStops[0], myStops.size(), &dxObject);
		return dxObject;
	}

	void Gradient::stops(Par<Array<GradientStop>> stops) {
		myStops = vector<D2D1_GRADIENT_STOP>(stops->count());
		for (nat i = 0; i < stops->count(); i++) {
			myStops[i].position = stops->at(i).pos;
			myStops[i].color = dx(stops->at(i).color);
		}
		destroy();
	}

	LinearGradient::LinearGradient(Par<Array<GradientStop>> stops, Angle angle) : Gradient(stops), angle(angle) {}

	void LinearGradient::create(Painter *owner, ID2D1Resource **out) {
		Point start, end;
		compute(Size(1, 1), start, end);
		D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES p = { dx(start), dx(end) };
		owner->renderTarget()->CreateLinearGradientBrush(p, dxStops(owner), (ID2D1LinearGradientBrush**)out);
	}

	void LinearGradient::prepare(const Rect &rect) {
		Point start, end;
		compute(rect.size(), start, end);
		start += rect.p0;
		end += rect.p0;
		if (ID2D1LinearGradientBrush *o = peek<ID2D1LinearGradientBrush>()) {
			o->SetStartPoint(dx(start));
			o->SetEndPoint(dx(end));
		}
	}

	void LinearGradient::compute(const Size &size, Point &start, Point &end) {
		Point dir = storm::geometry::angle(angle);
		Point c = center(size);

		float w = max(size.w, size.h);
		start = c + dir * -w;
		end = c + dir * w;
	}

}
