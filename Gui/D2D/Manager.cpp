#include "stdafx.h"
#include "Manager.h"
#include "Exception.h"

namespace gui {

#ifdef GUI_WIN32

	void D2DManager::check(HRESULT r, const wchar *msg) {
		if (FAILED(r)) {
			Str *m = TO_S(engine(), msg << ::toS(r).c_str());
			throw new (this) GuiError(m);
		}
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

	static void cleanupFn(void *ptr) {
		if (ptr) {
			IUnknown *u = (IUnknown *)ptr;
			u->Release();
		}
	}

	D2DManager::D2DManager(D2DSurface &surface) : surface(surface) {}

	void D2DManager::create(SolidBrush *brush, void *&result, Resource::Cleanup &cleanup) {
		ID2D1SolidColorBrush *b = null;
		check(surface.target()->CreateSolidColorBrush(dx(brush->color()), &b), S("Failed to create a brush"));
		b->SetOpacity(brush->opacity());
		result = b;
		cleanup = &cleanupFn;
	}

	void D2DManager::update(SolidBrush *brush, void *resource) {
		ID2D1SolidColorBrush *b = (ID2D1SolidColorBrush *)resource;
		b->SetOpacity(brush->opacity());
		b->SetColor(dx(brush->color()));

		throw new (this) NotSupported(S("update(SolidBrush)"));
	}

	void D2DManager::create(LinearGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		ID2D1GradientStopCollection *stops = createStops(surface.target(), brush->peekStops());

		D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES p = { dx(brush->start()), dx(brush->end()) };
		ID2D1LinearGradientBrush *b = null;
		HRESULT r = surface.target()->CreateLinearGradientBrush(p, stops, &b);
		if (stops)
			stops->Release();

		check(r, S("Failed to create linear gradient: "));

		result = b;
		cleanup = &cleanupFn;
	}

	void D2DManager::update(LinearGradient *brush, void *resource) {
		ID2D1LinearGradientBrush *b = (ID2D1LinearGradientBrush *)resource;
		b->SetStartPoint(dx(brush->start()));
		b->SetEndPoint(dx(brush->end()));
	}

	void D2DManager::create(RadialGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		ID2D1GradientStopCollection *stops = createStops(surface.target(), brush->peekStops());

		D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES p = { dx(brush->center()), { 0.0f, 0.0f }, brush->radius(), brush->radius() };
		D2D1_BRUSH_PROPERTIES p2 = { 1.0f, dx(brush->transform()) };
		ID2D1RadialGradientBrush *b = null;
		HRESULT r = surface.target()->CreateRadialGradientBrush(p, p2, stops, &b);
		stops->Release();

		check(r, S("Failed to create radial gradient: "));

		result = b;
		cleanup = &cleanupFn;
	}

	void D2DManager::update(RadialGradient *brush, void *resource) {
		ID2D1RadialGradientBrush *b = (ID2D1RadialGradientBrush *)resource;
		b->SetCenter(dx(brush->center()));
		b->SetRadiusX(brush->radius());
		b->SetRadiusY(brush->radius());
		b->SetTransform(dx(brush->transform()));
	}

#else

	DEFINE_GRAPHICS_MGR_FNS(D2DManager);

#endif
}
