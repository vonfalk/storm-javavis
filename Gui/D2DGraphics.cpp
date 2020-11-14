#include "stdafx.h"
#include "D2DGraphics.h"
#include "D2D.h"
#include "Painter.h"
#include "Brush.h"
#include "Text.h"
#include "Path.h"
#include "Bitmap.h"

namespace gui {

	D2DGraphics::D2DGraphics(D2DSurface &surface) : surface(surface) {
		oldStates = new (this) Array<State>();
		layers = new (this) Array<Layer>();

		state = State();
		oldStates->push(state);
		layerHistory = runtime::allocArray<Nat>(engine(), &natArrayType, layerHistoryCount);

		for (Nat i = 0; i < layerHistory->count; i++)
			layerHistory->v[i] = 0;
	}

	D2DGraphics::~D2DGraphics() {
		for (Nat i = 0; i < layers->count(); i++)
			layers->at(i).release();
	}

#ifdef GUI_WIN32

	void D2DGraphics::surfaceResized() {
		// Remove any layers.
		for (Nat i = 0; i < layers->count(); i++)
			layers->at(i).release();
		layers->clear();
	}

	void D2DGraphics::surfaceDestroyed() {
		// Clear the surface?

		// Remove any layers.
		for (Nat i = 0; i < layers->count(); i++)
			layers->at(i).release();
		layers->clear();
	}

	/**
	 * State management.
	 */

	void D2DGraphics::beforeRender(Color bgColor) {
		// Begin drawing, fill the background.
		surface.target()->BeginDraw();
		surface.target()->SetTransform(D2D1::Matrix3x2F::Identity());
		surface.target()->Clear(dx(bgColor));

		// Keep track of free layers so we can remove them if we have too many.
		minFreeLayers = layers->count();

		// Update the clip region and scale of the root state.
		state = State();
		state.scale(surface.scale);
		state.clip = Rect(Point(), surface.size);
		oldStates->last() = state;

		// Set up the backend.
		prepare();
	}

	bool D2DGraphics::afterRender() {
		HRESULT result = surface.target()->EndDraw();
		// Check if we need to re-create the device.
		bool ok = !(result == D2DERR_RECREATE_TARGET || result == DXGI_ERROR_DEVICE_RESET);

		// Make sure all layers are returned to the stack.
		reset();

		// Shift the states around.
		for (nat i = layerHistory->count - 1; i > 0; i--) {
			layerHistory->v[i] = layerHistory->v[i - 1];
		}
		layerHistory->v[0] = layers->count() - minFreeLayers;

		nat maxLayers = 0;
		for (nat i = 0; i < layerHistory->count; i++) {
			maxLayers = max(maxLayers, layerHistory->v[i]);
		}

		// Remove any layers that seems to not be needed in the near future.
		while (layers->count() > maxLayers) {
			layers->last().release();
			layers->pop();
		}

		return ok;
	}


	void D2DGraphics::reset() {
		// Do any PopLayer calls required.
		while (oldStates->count() > 1) {
			if (state.layer) {
				if (state.layer == Layer::dummy()) {
					surface.target()->PopAxisAlignedClip();
				} else {
					layers->push(state.layer);
					surface.target()->PopLayer();
				}
			}
			state = oldStates->last();
			oldStates->pop();
		}

		state = oldStates->at(0);
	}

	void D2DGraphics::prepare() {
		surface.target()->SetTransform(*state.transform());
	}

	Bool D2DGraphics::pop() {
		if (state.layer) {
			if (state.layer == Layer::dummy()) {
				surface.target()->PopAxisAlignedClip();
			} else {
				layers->push(state.layer);
				surface.target()->PopLayer();
			}
		}

		state = oldStates->last();
		surface.target()->SetTransform(*state.transform());
		if (oldStates->count() > 1) {
			oldStates->pop();
			return true;
		} else {
			return false;
		}
	}

	static D2D1_LAYER_PARAMETERS defaultParameters() {
		D2D1_LAYER_PARAMETERS p = {
			D2D1::InfiniteRect(),
			NULL,
			D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
			D2D1::IdentityMatrix(),
			1.0,
			NULL,
			D2D1_LAYER_OPTIONS_INITIALIZE_FOR_CLEARTYPE,
		};
		return p;
	}

	ID2D1Layer *D2DGraphics::layer() {
		ID2D1Layer *r = null;
		if (layers->count() > 0) {
			r = layers->last().v;
			layers->pop();
		} else {
			surface.target()->CreateLayer(NULL, &r);
		}

		minFreeLayers = min(minFreeLayers, layers->count());
		return r;
	}

	void D2DGraphics::push() {
		oldStates->push(state);
		state.layer = Layer();
	}

	void D2DGraphics::push(Float opacity) {
		// Optimization for cases when opacity >= 1.
		if (opacity >= 1.0f) {
			push();
			return;
		}

		oldStates->push(state);
		state.layer = layer();

		D2D1_LAYER_PARAMETERS p = defaultParameters();
		p.opacity = opacity;
		surface.target()->PushLayer(&p, state.layer.v);
	}

	void D2DGraphics::push(Rect clip) {
		oldStates->push(state);
		state.layer = Layer::dummy();

		surface.target()->PushAxisAlignedClip(dx(clip), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	}

	void D2DGraphics::push(Rect clip, Float opacity) {
		// Optimization for cases when opacity >= 1.
		if (opacity >= 1.0f) {
			push(clip);
			return;
		}

		oldStates->push(state);
		state.layer = layer();

		D2D1_LAYER_PARAMETERS p = defaultParameters();
		p.contentBounds = dx(clip);
		p.opacity = opacity;
		surface.target()->PushLayer(&p, state.layer.v);
	}

	void D2DGraphics::transform(Transform *tfm) {
		*state.transform() = dxMultiply(dx(tfm), *oldStates->last().transform());
		surface.target()->SetTransform(*state.transform());
	}

	void D2DGraphics::lineWidth(Float w) {
		// TODO: How is the width of lines affected by scaling etc.? How do we want it to behave?
		// TODO: PDF does not set the line width in relation to previous states. Maybe we should not either?
		state.lineWidth = oldStates->last().lineWidth * w;
	}

	/**
	 * Draw stuff.
	 */

	void D2DGraphics::line(Point from, Point to, Brush *style) {
		// surface.target()->DrawLine(dx(from), dx(to), style->brush(owner), state.lineWidth);
	}

	void D2DGraphics::draw(Rect rect, Brush *style) {
		// surface.target()->DrawRectangle(dx(rect), style->brush(owner), state.lineWidth);
	}

	void D2DGraphics::draw(Rect rect, Size edges, Brush *style) {
		// D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		// surface.target()->DrawRoundedRectangle(r, style->brush(owner), state.lineWidth);
	}

	void D2DGraphics::oval(Rect rect, Brush *style) {
		// Size s = rect.size() / 2;
		// D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		// surface.target()->DrawEllipse(e, style->brush(owner), state.lineWidth);
	}

	void D2DGraphics::draw(Path *path, Brush *style) {
		// surface.target()->DrawGeometry(path->geometry(), style->brush(owner), state.lineWidth);
	}

	void D2DGraphics::fill(Rect rect, Brush *style) {
		// surface.target()->FillRectangle(dx(rect), style->brush(owner));
	}

	void D2DGraphics::fill(Rect rect, Size edges, Brush *style) {
		// D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		// surface.target()->FillRoundedRectangle(r, style->brush(owner));
	}

	void D2DGraphics::fill(Brush *style) {
		// Rect s(Point(), surface.size / surface.scale);
		// surface.target()->SetTransform(D2D1::Matrix3x2F::Identity());
		// surface.target()->FillRectangle(dx(s), style->brush(owner));
		// surface.target()->SetTransform(*state.transform());
	}

	void D2DGraphics::fillOval(Rect rect, Brush *style) {
		// Size s = rect.size() / 2;
		// D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		// surface.target()->FillEllipse(e, style->brush(owner));
	}

	void D2DGraphics::fill(Path *path, Brush *style) {
		// surface.target()->FillGeometry(path->geometry(), style->brush(owner));
	}

	void D2DGraphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		// surface.target()->DrawBitmap(bitmap->bitmap(owner), &dx(rect), opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
	}

	void D2DGraphics::draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) {
		// surface.target()->DrawBitmap(bitmap->bitmap(owner), &dx(dest), opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &dx(src));
	}

	void D2DGraphics::text(Str *text, Font *font, Brush *style, Rect rect) {
		// ID2D1Brush *b = style->brush(owner);
		// surface.target()->DrawText(text->c_str(), text->peekLength(), font->textFormat(), dx(rect), b);
	}

	void D2DGraphics::draw(Text *text, Brush *style, Point origin) {
		// surface.target()->DrawTextLayout(dx(origin), text->layout(owner), style->brush(owner));
	}

	void D2DGraphics::Layer::release() {
		v->Release();
		v = null;
	}

#else

	DEFINE_WINDOW_GRAPHICS_FNS(CairoGraphics)

#endif

}
