#include "stdafx.h"
#include "Brush.h"
#include "Painter.h"

namespace stormgui {

	void Brush::prepare(const Size &s) {}

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

	LinearGradient::LinearGradient(Par<Array<GradientStop>> stops, Point start, Point end) :
		Gradient(stops), myStart(start), myEnd(end) {}

	void LinearGradient::create(Painter *owner, ID2D1Resource **out) {
		D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES p = { dx(myStart), dx(myEnd) };
		owner->renderTarget()->CreateLinearGradientBrush(p, dxStops(owner), (ID2D1LinearGradientBrush**)out);
	}

	void LinearGradient::prepare(const Size &size) {
		tfmStart.x = myStart.x * size.w;
		tfmStart.y = myStart.y * size.h;
		tfmEnd.x = myEnd.x * size.w;
		tfmEnd.y = myEnd.y * size.h;
		if (ID2D1LinearGradientBrush *o = peek<ID2D1LinearGradientBrush>()) {
			o->SetStartPoint(dx(tfmStart));
			o->SetEndPoint(dx(tfmEnd));
		}
	}

	Point LinearGradient::start() {
		return myStart;
	}

	Point LinearGradient::end() {
		return myEnd;
	}

	void LinearGradient::start(Point p) {
		myStart = p;
	}

	void LinearGradient::end(Point p) {
		myEnd = p;
	}

}
