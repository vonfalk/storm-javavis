#include "stdafx.h"
#include "Brush.h"

namespace gui {

	D2DBrush::D2DBrush() : brush(null) {}

	D2DBrush::~D2DBrush() {
		destroy();
	}

	void D2DBrush::destroy() {
		if (brush)
			brush->Release();
		brush = null;
	}


	D2DSolidBrush::D2DSolidBrush(D2DSurface &surface, SolidBrush *src) : src(src) {
		ID2D1SolidColorBrush *b = null;
		surface.target()->CreateSolidColorBrush(dx(src->color()), &b);
		b->SetOpacity(src->opacity());
		brush = b;
	}

	void D2DSolidBrush::update() {
		if (!brush)
			return;

		ID2D1SolidColorBrush *b = (ID2D1SolidColorBrush *)brush;
		b->SetOpacity(src->opacity());
		b->SetColor(dx(src->color()));
	}

	static ID2D1GradientStopCollection *createStops(ID2D1RenderTarget *context, Array<GradientStop> *stops) {
		D2D1_GRADIENT_STOP *tmp = (D2D1_GRADIENT_STOP *)alloca(sizeof(D2D1_GRADIENT_STOP)*stops->count());
		for (Nat i = 0; i < stops->count(); i++) {
			tmp[i].position = stops->at(i).pos;
			tmp[i].color = dx(stops->at(i).color);
		}

		ID2D1GradientStopCollection *result = null;
		context->CreateGradientStopCollection(tmp, stops->count(), &result);
		return result;
	}

	D2DLinearGradient::D2DLinearGradient(D2DSurface &surface, LinearGradient *src) : src(src) {
		ID2D1GradientStopCollection *stops = createStops(surface.target(), src->peekStops());

		D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES p = { dx(src->start()), dx(src->end()) };
		ID2D1LinearGradientBrush *b = null;
		surface.target()->CreateLinearGradientBrush(p, stops, &b);
		stops->Release();

		brush = b;
	}

	void D2DLinearGradient::update() {
		ID2D1LinearGradientBrush *b = (ID2D1LinearGradientBrush *)brush;
		b->SetStartPoint(dx(src->start()));
		b->SetEndPoint(dx(src->end()));
	}

	D2DRadialGradient::D2DRadialGradient(D2DSurface &surface, RadialGradient *src) : src(src) {
		ID2D1GradientStopCollection *stops = createStops(surface.target(), src->peekStops());

		D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES p = { dx(src->center()), { 0.0f, 0.0f }, src->radius(), src->radius() };
		D2D1_BRUSH_PROPERTIES p2 = { 1.0f, dx(src->transform()) };
		ID2D1RadialGradientBrush *b = null;
		surface.target()->CreateRadialGradientBrush(p, p2, stops, &b);
		stops->Release();

		brush = b;
	}

	void D2DRadialGradient::update() {
		ID2D1RadialGradientBrush *b = (ID2D1RadialGradientBrush *)brush;
		b->SetCenter(dx(src->center()));
		b->SetRadiusX(src->radius());
		b->SetRadiusY(src->radius());
		b->SetTransform(dx(src->transform()));
	}

}
