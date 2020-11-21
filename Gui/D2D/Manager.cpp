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

	D2DManager::D2DManager(Graphics *owner, D2DSurface &surface) : owner(owner), surface(&surface) {}

	void D2DManager::create(SolidBrush *brush, void *&result, Resource::Cleanup &cleanup) {
		ID2D1SolidColorBrush *b = null;
		check(surface->target()->CreateSolidColorBrush(dx(brush->color()), &b), S("Failed to create a solid brush: "));
		b->SetOpacity(brush->opacity());
		result = b;
		cleanup = &cleanupFn;
	}

	void D2DManager::update(SolidBrush *brush, void *resource) {
		ID2D1SolidColorBrush *b = (ID2D1SolidColorBrush *)resource;
		b->SetOpacity(brush->opacity());
		b->SetColor(dx(brush->color()));
	}

	void D2DManager::create(BitmapBrush *brush, void *&result, Resource::Cleanup &cleanup) {
		ID2D1Bitmap *bitmap = (ID2D1Bitmap *)brush->bitmap()->forGraphicsRaw(owner);

		D2D1_BITMAP_BRUSH_PROPERTIES props = {
			D2D1_EXTEND_MODE_WRAP,
			D2D1_EXTEND_MODE_WRAP,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
		};

		ID2D1BitmapBrush *b = null;
		check(surface->target()->CreateBitmapBrush(bitmap, props, &b), S("Failed to create bitmap brush: "));

		b->SetTransform(dx(brush->transform()));
		b->SetOpacity(brush->opacity());

		result = b;
		cleanup = &cleanupFn;
	}

	void D2DManager::update(BitmapBrush *brush, void *resource) {
		ID2D1BitmapBrush *b = (ID2D1BitmapBrush *)resource;
		b->SetOpacity(brush->opacity());
		b->SetTransform(dx(brush->transform()));
	}

	void D2DManager::create(LinearGradient *brush, void *&result, Resource::Cleanup &cleanup) {
		ID2D1GradientStopCollection *stops = createStops(surface->target(), brush->peekStops());

		D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES p = { dx(brush->start()), dx(brush->end()) };
		ID2D1LinearGradientBrush *b = null;
		HRESULT r = surface->target()->CreateLinearGradientBrush(p, stops, &b);
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
		ID2D1GradientStopCollection *stops = createStops(surface->target(), brush->peekStops());

		D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES p = { dx(brush->center()), { 0.0f, 0.0f }, brush->radius(), brush->radius() };
		D2D1_BRUSH_PROPERTIES p2 = { 1.0f, dx(brush->transform()) };
		ID2D1RadialGradientBrush *b = null;
		HRESULT r = surface->target()->CreateRadialGradientBrush(p, p2, stops, &b);
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

	void D2DManager::create(Bitmap *bitmap, void *&result, Resource::Cleanup &cleanup) {
		Nat w = bitmap->image()->width();
		Nat h = bitmap->image()->height();

		D2D1_SIZE_U s = { w, h };
		D2D1_BITMAP_PROPERTIES props = {
			{ DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
			96, 96 // DPI.
		};

		// Non-premultiplied alpha is not supported. We need to convert our source bitmap.
		byte *src = bitmap->image()->buffer();
		nat stride = bitmap->image()->stride();
		byte *img = runtime::allocBuffer(engine(), stride * h)->v;
		for (Nat y = 0; y < h; y++) {
			Nat base = stride * y;
			for (Nat x = 0; x < w; x++) {
				Nat p = base + x*4;
				float a = float(src[p+3]) / 255.0f;
				img[p+3] = src[p+3];
				img[p+0] = byte(src[p+0] * a);
				img[p+1] = byte(src[p+1] * a);
				img[p+2] = byte(src[p+2] * a);
			}
		}

		HRESULT r = surface->target()->CreateBitmap(s, img, stride, props, (ID2D1Bitmap **)&result);
		check(r, S("Failed to create bitmap:"));
		cleanup = &cleanupFn;
	}

	void D2DManager::update(Bitmap *, void *) {
		// Should never be called.
	}

	void D2DManager::create(Path *path, void *&result, Resource::Cleanup &cleanup) {
		ComPtr<ID2D1Factory> factory;
		surface->target()->GetFactory(&factory.v);

		ID2D1PathGeometry *geometry;
		check(factory->CreatePathGeometry(&geometry), S("Failed to create a path object: "));

		ID2D1GeometrySink *sink;
		check(geometry->Open(&sink), S("Failed to open the path: "));

		sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
		bool started = false;

		Array<PathPoint> *elements = path->peekData();
		for (Nat i = 0; i < elements->count(); i++) {
			PathPoint &e = elements->at(i);
			switch (e.t) {
			case tStart:
				if (started)
					sink->EndFigure(D2D1_FIGURE_END_OPEN);
				sink->BeginFigure(dx(e.start()->pt), D2D1_FIGURE_BEGIN_FILLED);
				started = true;
				break;
			case tClose:
				if (started)
					sink->EndFigure(D2D1_FIGURE_END_CLOSED);
				started = false;
				break;
			case tLine:
				if (started)
					sink->AddLine(dx(e.line()->to));
				break;
			case tBezier2:
				if (started) {
					D2D1_QUADRATIC_BEZIER_SEGMENT s = { dx(e.bezier2()->c1), dx(e.bezier2()->to) };
					sink->AddQuadraticBezier(s);
				}
				break;
			case tBezier3:
				if (started) {
					D2D1_BEZIER_SEGMENT s = { dx(e.bezier3()->c1), dx(e.bezier3()->c2), dx(e.bezier3()->to) };
					sink->AddBezier(s);
				}
				break;
			}
		}

		if (started)
			sink->EndFigure(D2D1_FIGURE_END_OPEN);

		sink->Close();
		sink->Release();

		result = geometry;
		cleanup = &cleanupFn;
	}

	void D2DManager::update(Path *, void *) {
		// Should never be called.
	}

#else

	DEFINE_GRAPHICS_MGR_FNS(D2DManager);

#endif
}
