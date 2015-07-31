#include "stdafx.h"
#include "Graphics.h"
#include "Painter.h"

namespace stormgui {

	Graphics::Graphics(ID2D1RenderTarget *target, Painter *p) : target(target), owner(p) {
		state = defaultState();
		oldStates.push_back(state);

		for (nat i = 0; i < layerHistory; i++)
			layerCount[i] = 0;
	}

	Graphics::~Graphics() {
		for (nat i = 0; i < layers.size(); i++)
			layers[i]->Release();
	}

	Size Graphics::size() {
		return convert(target->GetSize());
	}

	void Graphics::updateTarget(ID2D1RenderTarget *target) {
		this->target = target;

		// Remove any layers.
		for (nat i = 0; i < layers.size(); i++)
			layers[i]->Release();
		layers.clear();
	}

	void Graphics::destroyed() {
		target = null;
		owner = null;
	}

	/**
	 * State management.
	 */

	Graphics::State Graphics::defaultState() {
		State s = {
			dxUnit(),
			1.0f,
			null,
		};
		return s;
	}

	void Graphics::beforeRender() {
		minFreeLayers = layers.size();
	}

	void Graphics::afterRender() {
		// Make sure all layers are returned to the stack.
		reset();

		// Shift the states around.
		for (nat i = layerHistory - 1; i > 0; i--) {
			layerCount[i] = layerCount[i - 1];
		}
		layerCount[0] = layers.size() - minFreeLayers;

		nat maxLayers = 0;
		for (nat i = 0; i < layerHistory; i++) {
			maxLayers = max(maxLayers, layerCount[i]);
		}

		// Remove any layers that seems to not be needed in the near future.
		while (layers.size() > maxLayers) {
			layers.back()->Release();
			layers.pop_back();
		}
	}

	void Graphics::reset() {
		// Do any PopLayer calls required.
		while (oldStates.size() > 1) {
			if (state.layer) {
				layers.push_back(state.layer);
				target->PopLayer();
			}
			state = oldStates.back();
			oldStates.pop_back();
		}

		state = oldStates[0];
	}

	Bool Graphics::pop() {
		if (state.layer) {
			layers.push_back(state.layer);
			target->PopLayer();
		}

		state = oldStates.back();
		target->SetTransform(state.transform);
		if (oldStates.size() > 1) {
			oldStates.pop_back();
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
		if (layers.size() > 0) {
			r = layers.back();
			layers.pop_back();
		} else {
			target->CreateLayer(NULL, &r);
		}

		minFreeLayers = min(minFreeLayers, layers.size());
		return r;
	}

	void Graphics::push() {
		oldStates.push_back(state);
		state.layer = null;
	}

	void Graphics::push(Float opacity) {
		oldStates.push_back(state);
		state.layer = layer();

		D2D1_LAYER_PARAMETERS p = defaultParameters();
		p.opacity = opacity;
		target->PushLayer(&p, state.layer);
	}

	void Graphics::push(Rect clip) {
		oldStates.push_back(state);
		state.layer = layer();

		D2D1_LAYER_PARAMETERS p = defaultParameters();
		p.contentBounds = dx(clip);
		target->PushLayer(&p, state.layer);
	}

	void Graphics::push(Rect clip, Float opacity) {
		oldStates.push_back(state);
		state.layer = layer();

		D2D1_LAYER_PARAMETERS p = defaultParameters();
		p.contentBounds = dx(clip);
		p.opacity = opacity;
		target->PushLayer(&p, state.layer);
	}

	void Graphics::transform(Par<Transform> tfm) {
		state.transform = dxMultiply(dx(tfm), oldStates.back().transform);
		target->SetTransform(state.transform);
	}

	void Graphics::lineWidth(Float w) {
		state.lineWidth = oldStates.back().lineWidth * w;
	}

	/**
	 * Draw stuff.
	 */

	void Graphics::line(Point from, Point to, Par<Brush> style) {
		target->DrawLine(dx(from), dx(to), style->brush(owner, Rect(from, to).normalized()), state.lineWidth);
	}

	void Graphics::draw(Rect rect, Par<Brush> style) {
		target->DrawRectangle(dx(rect), style->brush(owner, rect), state.lineWidth);
	}

	void Graphics::draw(Rect rect, Size edges, Par<Brush> style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		target->DrawRoundedRectangle(r, style->brush(owner, rect), state.lineWidth);
	}

	void Graphics::oval(Rect rect, Par<Brush> style) {
		Size s = rect.size() / 2;
		D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		target->DrawEllipse(e, style->brush(owner, rect), state.lineWidth);
	}

	void Graphics::draw(Par<Path> path, Par<Brush> brush) {
		target->DrawGeometry(path->geometry(), brush->brush(owner, path->bound()), state.lineWidth);
	}

	void Graphics::fill(Rect rect, Par<Brush> style) {
		target->FillRectangle(dx(rect), style->brush(owner, rect));
	}

	void Graphics::fill(Rect rect, Size edges, Par<Brush> style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		target->FillRoundedRectangle(r, style->brush(owner, rect));
	}

	void Graphics::fill(Par<Brush> brush) {
		Rect s = Rect(Point(), size());
		target->SetTransform(D2D1::Matrix3x2F::Identity());
		target->FillRectangle(dx(s), brush->brush(owner, s));
		target->SetTransform(state.transform);
	}

	void Graphics::fillOval(Rect rect, Par<Brush> style) {
		Size s = rect.size() / 2;
		D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		target->FillEllipse(e, style->brush(owner, rect));
	}

	void Graphics::fill(Par<Path> path, Par<Brush> brush) {
		target->FillGeometry(path->geometry(), brush->brush(owner, path->bound()));
	}

	void Graphics::draw(Par<Bitmap> bitmap) {
		draw(bitmap, Point());
	}

	void Graphics::draw(Par<Bitmap> bitmap, Point topLeft) {
		draw(bitmap, Rect(topLeft, topLeft + bitmap->size()));
	}

	void Graphics::draw(Par<Bitmap> bitmap, Point topLeft, Float opacity) {
		draw(bitmap, Rect(topLeft, topLeft + bitmap->size()), opacity);
	}

	void Graphics::draw(Par<Bitmap> bitmap, Rect rect) {
		draw(bitmap, rect, 1);
	}

	void Graphics::draw(Par<Bitmap> bitmap, Rect rect, Float opacity) {
		target->DrawBitmap(bitmap->bitmap(owner), &dx(rect), opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
	}

	void Graphics::text(Par<Str> text, Par<Font> font, Par<Brush> brush, Rect rect) {
		ID2D1Brush *b = brush->brush(owner, rect);
		target->DrawText(text->v.c_str(), text->v.size(), font->textFormat(), dx(rect), b);
	}

	void Graphics::draw(Par<Text> text, Par<Brush> brush, Point origin) {
		target->DrawTextLayout(dx(origin), text->layout(), brush->brush(owner, Rect(origin, text->size())));
	}

}
