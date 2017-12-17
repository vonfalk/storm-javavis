#include "stdafx.h"
#include "Brush.h"
#include "Painter.h"
#include "Bitmap.h"

namespace gui {

	Brush::Brush() : opacity(1.0f) {}

#ifdef GUI_WIN32

	void Brush::prepare(const Rect &r, ID2D1Brush *b) {}

#endif
#ifdef GUI_GTK

	void Brush::stroke(Painter *p, NVGcontext *c, const Rect &bound) {
		nvgGlobalAlpha(c, opacity);
		setStroke(p, c, bound);
		nvgStroke(c);
	}

	void Brush::fill(Painter *p, NVGcontext *c, const Rect &bound) {
		nvgGlobalAlpha(c, opacity);
		setFill(p, c, bound);
		nvgFill(c);
	}

	void Brush::setStroke(Painter *p, NVGcontext *c, const Rect &bound) {}

	void Brush::setFill(Painter *p, NVGcontext *c, const Rect &bound) {}

#endif

	SolidBrush::SolidBrush(Color c) : color(c) {}

#ifdef GUI_WIN32

	void SolidBrush::create(Painter *owner, ID2D1Resource **out) {
		owner->renderTarget()->CreateSolidColorBrush(dx(color), (ID2D1SolidColorBrush **)out);
	}

#endif
#ifdef GUI_GTK

	void SolidBrush::setStroke(Painter *p, NVGcontext *c, const Rect &bound) {
		nvgStrokeColor(c, nvg(color));
	}

	void SolidBrush::setFill(Painter *p, NVGcontext *c, const Rect &bound) {
		nvgFillColor(c, nvg(color));
	}

#endif

	// BitmapBrush::BitmapBrush(Bitmap *bitmap) : bitmap(bitmap) {}

	// void BitmapBrush::create(Painter *owner, ID2D1Resource **out) {
	// 	D2D1_BITMAP_BRUSH_PROPERTIES props = {
	// 		D2D1_EXTEND_MODE_WRAP,
	// 		D2D1_EXTEND_MODE_WRAP,
	// 		D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
	// 	};
	// 	owner->renderTarget()->CreateBitmapBrush(bitmap->bitmap(owner), props, (ID2D1BitmapBrush **)out);
	// }

	// void BitmapBrush::prepare(const Rect &bound, ID2D1Brush *r) {
	// 	r->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(bound.p0.x, bound.p0.y)));
	// }

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

	void Gradient::setStroke(Painter *p, NVGcontext *c, const Rect &bound) {
		if (myStops->count() >= 2)
			nvgStrokePaint(c, createPaint(p, c, bound));
		else if (myStops->count() >= 1)
			nvgStrokeColor(c, nvg(myStops->at(0).color));
		else
			nvgStrokeColor(c, nvgRGBAf(0, 0, 0, 0));
	}

	void Gradient::setFill(Painter *p, NVGcontext *c, const Rect &bound) {
		if (myStops->count() >= 2)
			nvgFillPaint(c, createPaint(p, c, bound));
		else if (myStops->count() >= 1)
			nvgFillColor(c, nvg(myStops->at(0).color));
		else
			nvgFillColor(c, nvgRGBAf(0, 0, 0, 0));
	}

	NVGpaint Gradient::createPaint(Painter *p, NVGcontext *c, const Rect &bound) {
		assert(false);
		NVGpaint paint = {};
		return paint;
	}

#endif

	LinearGradient::LinearGradient(Array<GradientStop> *stops, Angle angle) : Gradient(stops), angle(angle) {}

	LinearGradient::LinearGradient(Color a, Color b, Angle angle) : Gradient(), angle(angle) {
		Array<GradientStop> *s = CREATE(Array<GradientStop>, this);
		s->push(GradientStop(0, a));
		s->push(GradientStop(1, b));
		stops(s);
	}

#ifdef GUI_WIN32

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

#endif
#ifdef GUI_GTK

	namespace texture {
		// A resolution of 128 should be enough since we'll be using linear interpolation on the texture.
		static const Nat width = 128;

		static Color lerp(Float x, const Color &a, const Color &b) {
			Float y = 1.0f - x;
			return Color(a.r*y + b.r*x, a.g*y + b.g*x, a.b*y + b.b*x, a.a*y + b.a*x);
		}

		static void write(byte *to, const Color &c) {
			to[0] = byte(c.r * 255.0f);
			to[1] = byte(c.g * 255.0f);
			to[2] = byte(c.b * 255.0f);
			to[3] = byte(c.a * 255.0f);
		}

		static void fill(byte *data, Float start, Float end, const Color &from, const Color &to) {
			if (start >= end)
				return;

			int startI = int(start * width);
			int endI = int(end * width);
			int delta = endI - startI;
			for (int i = 0; i < delta; i++) {
				Color c = lerp(Float(i) / delta, from, to);
				write(data + (startI + i)*4, c);
			}
		}

		static int create(NVGcontext *c, Array<GradientStop> *stops) {
			byte data[width*4];

			// Fill from zero until the first stop.
			{
				const GradientStop &first = stops->first();
				fill(data, 0.0f, first.pos, first.color, first.color);
			}

			// Fill the space between each step.
			for (Nat i = 0; i < stops->count() - 1; i++) {
				const GradientStop &from = stops->at(i);
				const GradientStop &to = stops->at(i + 1);
				fill(data, from.pos, to.pos, from.color, to.color);
			}

			// Fill from the last stop until the end.
			{
				const GradientStop &last = stops->last();
				fill(data, last.pos, 1.0f, last.color, last.color);
			}

			// Create the texture.
			return nvgCreateImageRGBA(c, 1, width, 0, data);
		}
	}

	NVGpaint LinearGradient::createPaint(Painter *p, NVGcontext *c, const Rect &bound) {
		Point start, end;
		Float distance;
		compute(bound, start, end, distance);

		if (myStops->count() == 2) {
			// Use the simple approach.
			GradientStop from = myStops->at(0);
			GradientStop to = myStops->at(1);

			// Adjust the points to the position of the gradient stops.
			Point dir = end - start;
			end = start + dir*to.pos;
			start = start + dir*from.pos;

			return nvgLinearGradient(c, start.x, start.y, end.x, end.y, nvg(from.color), nvg(to.color));
		}

		// We need to create a texture, and use that.
		int texId = texture(p);

		// Figure out where we want to draw the texture.
		Point perp = storm::geometry::angle(angle - deg(90));
		Point origin = start + perp * distance/2;

		return nvgImagePattern(c, origin.x, origin.y, distance, distance, (angle + deg(180)).rad(), texId, 1.0f);
	}

	int LinearGradient::create(Painter *owner) {
		return gui::texture::create(owner->nvgContext(), myStops);
	}

#endif

	void LinearGradient::compute(const Rect &bound, Point &start, Point &end) {
		Float d;
		compute(bound, start, end, d);
	}

	void LinearGradient::compute(const Rect &bound, Point &start, Point &end, Float &w) {
		Size size = bound.size();
		Point dir = storm::geometry::angle(angle);
		Point c = center(size);

		w = max(size.w, size.h);
		start = c + dir * -w;
		end = c + dir * w;

		start += bound.p0;
		end += bound.p0;
		w *= 2;
	}

}
