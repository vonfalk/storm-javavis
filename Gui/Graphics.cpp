#include "stdafx.h"
#include "Graphics.h"
#include "Painter.h"
#include "Brush.h"
#include "Text.h"
#include "Path.h"
#include "Bitmap.h"


namespace gui {

	Graphics::Graphics(ID2D1RenderTarget *target, Painter *p) : target(target), owner(p) {
		oldStates = new (this) Array<State>();
		layers = new (this) Array<Layer>();

		state = defaultState();
		oldStates->push(state);
		layerHistory = runtime::allocArray<Nat>(engine(), &natArrayType, layerHistoryCount);

		for (nat i = 0; i < layerHistory->count; i++)
			layerHistory->v[i] = 0;
	}

	Graphics::~Graphics() {
		for (nat i = 0; i < layers->count(); i++)
			layers->at(i).v->Release();
	}

	Size Graphics::size() {
		return convert(target->GetSize());
	}

	void Graphics::updateTarget(ID2D1RenderTarget *target) {
		this->target = target;

		// Remove any layers.
		// for (nat i = 0; i < layers.size(); i++)
		// 	layers->at(i)->Release();
		// layers.clear();
	}

	void Graphics::destroyed() {
		target = null;
		owner = null;
	}

	/**
	 * State management.
	 */

	Graphics::State Graphics::defaultState() {
		return State(dxUnit(), 1.0f);
	}

	void Graphics::beforeRender() {
		minFreeLayers = layers->count();
	}

	void Graphics::afterRender() {
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
			layers->last().v->Release();
			layers->pop();
		}
	}

	void Graphics::reset() {
		// Do any PopLayer calls required.
		while (oldStates->count() > 1) {
			if (state.layer.v) {
				layers->push(state.layer.v);
				target->PopLayer();
			}
			state = oldStates->last();
			oldStates->pop();
		}

		state = oldStates->at(0);
	}

	Bool Graphics::pop() {
		if (state.layer.v) {
			layers->push(state.layer);
			target->PopLayer();
		}

		state = oldStates->last();
		target->SetTransform(*state.transform());
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

	ID2D1Layer *Graphics::layer() {
		ID2D1Layer *r = null;
		if (layers->count() > 0) {
			r = layers->last().v;
			layers->pop();
		} else {
			target->CreateLayer(NULL, &r);
		}

		minFreeLayers = min(minFreeLayers, layers->count());
		return r;
	}

	void Graphics::push() {
		oldStates->push(state);
		state.layer = null;
	}

	void Graphics::push(Float opacity) {
		oldStates->push(state);
		state.layer = layer();

		D2D1_LAYER_PARAMETERS p = defaultParameters();
		p.opacity = opacity;
		target->PushLayer(&p, state.layer.v);
	}

	void Graphics::push(Rect clip) {
		oldStates->push(state);
		state.layer = layer();

		D2D1_LAYER_PARAMETERS p = defaultParameters();
		p.contentBounds = dx(clip);
		target->PushLayer(&p, state.layer.v);
	}

	void Graphics::push(Rect clip, Float opacity) {
		oldStates->push(state);
		state.layer = layer();

		D2D1_LAYER_PARAMETERS p = defaultParameters();
		p.contentBounds = dx(clip);
		p.opacity = opacity;
		target->PushLayer(&p, state.layer.v);
	}

	void Graphics::transform(Transform *tfm) {
		*state.transform() = dxMultiply(dx(tfm), *oldStates->last().transform());
		target->SetTransform(*state.transform());
	}

	void Graphics::lineWidth(Float w) {
		state.lineWidth = oldStates->last().lineWidth * w;
	}

	/**
	 * Draw stuff.
	 */

	void Graphics::line(Point from, Point to, Brush *style) {
		target->DrawLine(dx(from), dx(to), style->brush(owner, Rect(from, to).normalized()), state.lineWidth);
	}

	void Graphics::draw(Rect rect, Brush *style) {
		target->DrawRectangle(dx(rect), style->brush(owner, rect), state.lineWidth);
	}

	void Graphics::draw(Rect rect, Size edges, Brush *style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		target->DrawRoundedRectangle(r, style->brush(owner, rect), state.lineWidth);
	}

	void Graphics::oval(Rect rect, Brush *style) {
		Size s = rect.size() / 2;
		D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		target->DrawEllipse(e, style->brush(owner, rect), state.lineWidth);
	}

	void Graphics::draw(Path *path, Brush *brush) {
		target->DrawGeometry(path->geometry(), brush->brush(owner, path->bound()), state.lineWidth);
	}

	void Graphics::fill(Rect rect, Brush *style) {
		target->FillRectangle(dx(rect), style->brush(owner, rect));
	}

	void Graphics::fill(Rect rect, Size edges, Brush *style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		target->FillRoundedRectangle(r, style->brush(owner, rect));
	}

	void Graphics::fill(Brush *brush) {
		Rect s = Rect(Point(), size());
		target->SetTransform(D2D1::Matrix3x2F::Identity());
		target->FillRectangle(dx(s), brush->brush(owner, s));
		target->SetTransform(*state.transform());
	}

	void Graphics::fillOval(Rect rect, Brush *style) {
		Size s = rect.size() / 2;
		D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		target->FillEllipse(e, style->brush(owner, rect));
	}

	void Graphics::fill(Path *path, Brush *brush) {
		target->FillGeometry(path->geometry(), brush->brush(owner, path->bound()));
	}

	void Graphics::draw(Bitmap *bitmap) {
		draw(bitmap, Point());
	}

	void Graphics::draw(Bitmap *bitmap, Point topLeft) {
		draw(bitmap, Rect(topLeft, topLeft + bitmap->size()));
	}

	void Graphics::draw(Bitmap *bitmap, Point topLeft, Float opacity) {
		draw(bitmap, Rect(topLeft, topLeft + bitmap->size()), opacity);
	}

	void Graphics::draw(Bitmap *bitmap, Rect rect) {
		draw(bitmap, rect, 1);
	}

	void Graphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		target->DrawBitmap(bitmap->bitmap(owner), &dx(rect), opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
	}

	void Graphics::text(Str *text, Font *font, Brush *brush, Rect rect) {
		ID2D1Brush *b = brush->brush(owner, rect);
		target->DrawText(text->c_str(), text->peekLength(), font->textFormat(), dx(rect), b);
	}

	void Graphics::draw(Text *text, Brush *brush, Point origin) {
		target->DrawTextLayout(dx(origin), text->layout(), brush->brush(owner, Rect(origin, text->size())));
	}

}
