#include "stdafx.h"
#include "Graphics.h"
#include "Painter.h"
#include "Brush.h"
#include "Text.h"
#include "Path.h"
#include "Bitmap.h"


namespace gui {

	Graphics::Graphics(RenderInfo info, Painter *p) : info(info), owner(p) {
		oldStates = new (this) Array<State>();
		layers = new (this) Array<Layer>();

		state = State();
		oldStates->push(state);
		layerHistory = runtime::allocArray<Nat>(engine(), &natArrayType, layerHistoryCount);

		for (Nat i = 0; i < layerHistory->count; i++)
			layerHistory->v[i] = 0;
	}

	Graphics::~Graphics() {
		for (Nat i = 0; i < layers->count(); i++)
			layers->at(i).release();
	}

	void Graphics::updateTarget(RenderInfo info) {
		this->info = info;

		// Remove any layers.
		for (Nat i = 0; i < layers->count(); i++)
			layers->at(i).release();
		layers->clear();
	}

	Size Graphics::size() {
		return info.size;
	}

	void Graphics::destroyed() {
		info = RenderInfo();
		owner = null;

		// Remove any layers.
		for (Nat i = 0; i < layers->count(); i++)
			layers->at(i).release();
		layers->clear();
	}

	/**
	 * State management.
	 */

	void Graphics::beforeRender() {
		// Keep track of free layers so we can remove them if we have too many.
		minFreeLayers = layers->count();

		// Update the clip region of the root state.
		state.clip = Rect(Point(), info.size);
		oldStates->last().clip = state.clip;

		// Set up the backend.
		prepare();
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
			layers->last().release();
			layers->pop();
		}
	}

	/**
	 * Drawing shared between backends.
	 */

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

#ifdef GUI_WIN32

	void Graphics::reset() {
		// Do any PopLayer calls required.
		while (oldStates->count() > 1) {
			if (state.layer.v) {
				layers->push(state.layer);
				info.target()->PopLayer();
			}
			state = oldStates->last();
			oldStates->pop();
		}

		state = oldStates->at(0);
	}

	void Graphics::prepare() {
		info.target()->SetTransform(*state.transform());
	}

	Bool Graphics::pop() {
		if (state.layer.v) {
			layers->push(state.layer);
			info.target()->PopLayer();
		}

		state = oldStates->last();
		info.target()->SetTransform(*state.transform());
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
			info.target()->CreateLayer(NULL, &r);
		}

		minFreeLayers = min(minFreeLayers, layers->count());
		return r;
	}

	void Graphics::push() {
		oldStates->push(state);
		state.layer = Layer();
	}

	void Graphics::push(Float opacity) {
		oldStates->push(state);
		state.layer = layer();

		D2D1_LAYER_PARAMETERS p = defaultParameters();
		p.opacity = opacity;
		info.target()->PushLayer(&p, state.layer.v);
	}

	void Graphics::push(Rect clip) {
		oldStates->push(state);
		state.layer = layer();

		D2D1_LAYER_PARAMETERS p = defaultParameters();
		p.contentBounds = dx(clip);
		info.target()->PushLayer(&p, state.layer.v);
	}

	void Graphics::push(Rect clip, Float opacity) {
		oldStates->push(state);
		state.layer = layer();

		D2D1_LAYER_PARAMETERS p = defaultParameters();
		p.contentBounds = dx(clip);
		p.opacity = opacity;
		info.target()->PushLayer(&p, state.layer.v);
	}

	void Graphics::transform(Transform *tfm) {
		*state.transform() = dxMultiply(dx(tfm), *oldStates->last().transform());
		info.target()->SetTransform(*state.transform());
	}

	void Graphics::lineWidth(Float w) {
		// Note: How is the line size affected by scaling etc?
		state.lineWidth = oldStates->last().lineWidth * w;
	}

	/**
	 * Draw stuff.
	 */

	void Graphics::line(Point from, Point to, Brush *style) {
		info.target()->DrawLine(dx(from), dx(to), style->brush(owner, Rect(from, to).normalized()), state.lineWidth);
	}

	void Graphics::draw(Rect rect, Brush *style) {
		info.target()->DrawRectangle(dx(rect), style->brush(owner, rect), state.lineWidth);
	}

	void Graphics::draw(Rect rect, Size edges, Brush *style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		info.target()->DrawRoundedRectangle(r, style->brush(owner, rect), state.lineWidth);
	}

	void Graphics::oval(Rect rect, Brush *style) {
		Size s = rect.size() / 2;
		D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		info.target()->DrawEllipse(e, style->brush(owner, rect), state.lineWidth);
	}

	void Graphics::draw(Path *path, Brush *style) {
		info.target()->DrawGeometry(path->geometry(), style->brush(owner, path->bound()), state.lineWidth);
	}

	void Graphics::fill(Rect rect, Brush *style) {
		info.target()->FillRectangle(dx(rect), style->brush(owner, rect));
	}

	void Graphics::fill(Rect rect, Size edges, Brush *style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		info.target()->FillRoundedRectangle(r, style->brush(owner, rect));
	}

	void Graphics::fill(Brush *style) {
		Rect s = Rect(Point(), size());
		info.target()->SetTransform(D2D1::Matrix3x2F::Identity());
		info.target()->FillRectangle(dx(s), style->brush(owner, s));
		info.target()->SetTransform(*state.transform());
	}

	void Graphics::fillOval(Rect rect, Brush *style) {
		Size s = rect.size() / 2;
		D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		info.target()->FillEllipse(e, style->brush(owner, rect));
	}

	void Graphics::fill(Path *path, Brush *style) {
		info.target()->FillGeometry(path->geometry(), style->brush(owner, path->bound()));
	}

	void Graphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		info.target()->DrawBitmap(bitmap->bitmap(owner), &dx(rect), opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
	}

	void Graphics::text(Str *text, Font *font, Brush *style, Rect rect) {
		ID2D1Brush *b = style->brush(owner, rect);
		info.target()->DrawText(text->c_str(), text->peekLength(), font->textFormat(), dx(rect), b);
	}

	void Graphics::draw(Text *text, Brush *style, Point origin) {
		info.target()->DrawTextLayout(dx(origin), text->layout(), style->brush(owner, Rect(origin, text->size())));
	}

	void Graphics::Layer::release() {
		v->Release();
		v = null;
	}

#endif
#ifdef GUI_GTK

	/**
	 * State management.
	 */

	void Graphics::reset() {
		// Clear any remaining states from the stack.
		while (pop())
			;
	}

	TextureContext *Graphics::layer() {
		TextureContext *r = null;
		if (layers->count() > 0) {
			r = layers->last().v;
			layers->pop();
			// Resize to fit the screen if necessary.
			r->resize(info.size);
		} else {
			r = new TextureContext(info.context(), info.size);
		}

		// Clear the layer.
		r->activate();
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		minFreeLayers = min(minFreeLayers, layers->count());
		return r;
	}

	void Graphics::push() {
		oldStates->push(state);
	}

	void Graphics::push(Float opacity) {
		oldStates->push(state);

		nvgFlush(info.context()->nvg);
		state.layer = layer();
		state.opacity = opacity;
	}

	void Graphics::push(Rect clip) {
		oldStates->push(state);

		nvgFlush(info.context()->nvg);
		state.layer = layer();
		state.opacity = 1.0f;
		state.clip = clip;
	}

	void Graphics::push(Rect clip, Float opacity) {
		oldStates->push(state);

		nvgFlush(info.context()->nvg);
		state.layer = layer();
		state.opacity = opacity;
		state.clip = clip;
	}

	Bool Graphics::pop() {
		State prev = state;
		state = oldStates->last();
		prepare();

		// See if we need to apply and dispose a layer.
		if (prev.layer && prev.layer != state.layer) {
			TextureContext *gl = prev.layer.v;
			nvgFlush(gl->nvg);

			// Draw the previous layer onto the now current layer, using the opacity present here.
			// The texture inside layers are always the same size and position as the main rendering
			// target. Therefore, we want to draw it in (0, 0) - (w, h). However, we probably want
			// to do that having the proper transformation applied so that we can clip properly if
			// we need to.
			NVGcontext *c = context();

			// Set the clip region.
			Size clipSize = prev.clip.size();
			nvgScissor(c, prev.clip.p0.x, prev.clip.p0.y, clipSize.w, clipSize.h);

			// Draw a rectangle at (0, 0) - (w, h) in real coordinates. Thus, we need to negate the
			// current transform.
			float tfm[6];
			nvgCurrentTransform(c, tfm);
			float invTfm[6];
			nvgTransformInverse(invTfm, tfm);
			float x, y;
			nvgBeginPath(c);
			nvgTransformPoint(&x, &y, invTfm, 0, 0);
			nvgMoveTo(c, x, y);
			nvgTransformPoint(&x, &y, invTfm, info.size.w, 0);
			nvgLineTo(c, x, y);
			nvgTransformPoint(&x, &y, invTfm, info.size.w, info.size.h);
			nvgLineTo(c, x, y);
			nvgTransformPoint(&x, &y, invTfm, 0, info.size.h);
			nvgLineTo(c, x, y);
			nvgClosePath(c);

			// Draw the image.
			nvgFillPaint(c, nvgImagePatternRaw(c, invTfm, info.size.w, info.size.h, gl->nvgImage(), prev.opacity));
			nvgFill(c);

			// Reset clip region.
			nvgResetScissor(c);

			// Push it to the layer stack for later reuse.
			layers->push(prev.layer);
		}

		if (oldStates->count() > 1) {
			oldStates->pop();
			return true;
		} else {
			return false;
		}
	}

	void Graphics::transform(Transform *tfm) {
		NVGcontext *c = info.context()->nvg;
		nvgSetTransform(c, oldStates->last().transform());
		nvgTransform(c, tfm);
		nvgCurrentTransform(c, state.transform());
	}

	void Graphics::lineWidth(Float w) {
		// TODO: How is the width of lines affected by scaling etc.?
		state.lineWidth = oldStates->last().lineWidth * w;
		nvgStrokeWidth(info.context()->nvg, state.lineWidth);
	}

	void Graphics::prepare() {
		NVGcontext *c = info.context()->nvg;
		nvgSetTransform(c, state.transform());
		nvgStrokeWidth(c, state.lineWidth);
	}

	void Graphics::Layer::release() {
		delete v;
		v = null;
	}

	NVGcontext *Graphics::context() {
		// Find the context to activate and make sure it is active. There might have been a thread
		// switch somewhere since last time.

		GlContext *c = info.context();
		// If the current state contains a TextureContext, use that.
		if (state.layer)
			c = state.layer.v;

		assert(c);
		c->activate();

		return c->nvg;
	}

	/**
	 * Draw stuff.
	 */

	void Graphics::line(Point from, Point to, Brush *style) {
		NVGcontext *c = context();
		nvgBeginPath(c);
		nvgMoveTo(c, from.x, from.y);
		nvgLineTo(c, to.x, to.y);
		style->stroke(owner, c, Rect(from, to).normalized());
	}

	void Graphics::draw(Rect rect, Brush *style) {
		NVGcontext *c = context();
		nvgBeginPath(c);
		Size sz = rect.size();
		nvgRect(c, rect.p0.x, rect.p0.y, sz.w, sz.h);
		style->stroke(owner, c, rect);
	}

	static void roundedRect(NVGcontext *c, Rect rect, Size edges) {
		const float kappa90 = 0.5522847493f;
		const float invKappa90 = 1.0f - kappa90;

		nvgMoveTo(c, rect.p1.x - edges.w, rect.p0.y);
		nvgBezierTo(c,
					rect.p1.x - edges.w*invKappa90, rect.p0.y,
					rect.p1.x, rect.p0.y + edges.h*invKappa90,
					rect.p1.x, rect.p0.y + edges.h);
		nvgLineTo(c, rect.p1.x, rect.p1.y - edges.h);
		nvgBezierTo(c,
					rect.p1.x, rect.p1.y - edges.h*invKappa90,
					rect.p1.x - edges.w*invKappa90, rect.p1.y,
					rect.p1.x - edges.w, rect.p1.y);
		nvgLineTo(c, rect.p0.x + edges.w, rect.p1.y);
		nvgBezierTo(c,
					rect.p0.x + edges.w*invKappa90, rect.p1.y,
					rect.p0.x, rect.p1.y - edges.h*invKappa90,
					rect.p0.x, rect.p1.y - edges.h);
		nvgLineTo(c, rect.p0.x, rect.p0.y + edges.h);
		nvgBezierTo(c,
					rect.p0.x, rect.p0.y + edges.h*invKappa90,
					rect.p0.x + edges.w*invKappa90, rect.p0.y,
					rect.p0.x + edges.w, rect.p0.y);
		nvgClosePath(c);
	}

	void Graphics::draw(Rect rect, Size edges, Brush *style) {
		NVGcontext *c = context();
		nvgBeginPath(c);
		roundedRect(c, rect, edges);
		style->stroke(owner, c, rect);
	}

	static void drawOval(NVGcontext *c, Rect rect) {
		Size size = rect.size() / 2;
		Point center = rect.center();
		nvgEllipse(c, center.x, center.y, size.w, size.h);
	}

	void Graphics::oval(Rect rect, Brush *style) {
		NVGcontext *c = context();
		nvgBeginPath(c);
		drawOval(c, rect);
		style->stroke(owner, c, rect);
	}

	void Graphics::draw(Path *path, Brush *style) {
		NVGcontext *c = context();
		path->draw(c);
		style->stroke(owner, c, path->bound());
	}

	void Graphics::fill(Rect rect, Brush *style) {
		NVGcontext *c = context();
		nvgBeginPath(c);
		Size sz = rect.size();
		nvgRect(c, rect.p0.x, rect.p0.y, sz.w, sz.h);
		style->fill(owner, c, rect);
	}

	void Graphics::fill(Rect rect, Size edges, Brush *style) {
		NVGcontext *c = context();
		nvgBeginPath(c);
		roundedRect(c, rect, edges);
		style->fill(owner, c, rect);
	}

	void Graphics::fill(Brush *style) {
		NVGcontext *c = context();
		nvgResetTransform(c);
		Size sz = size();
		nvgBeginPath(c);
		nvgRect(c, 0, 0, sz.w + 1.0f, sz.h + 1.0f);
		style->fill(owner, c, Rect(Point(), sz));
		// Go back to the previous transform.
		nvgSetTransform(c, state.transform());
	}

	void Graphics::fillOval(Rect rect, Brush *style) {
		NVGcontext *c = context();
		nvgBeginPath(c);
		drawOval(c, rect);
		style->fill(owner, c, rect);
	}

	void Graphics::fill(Path *path, Brush *style) {
		NVGcontext *c = context();
		path->draw(c);
		style->fill(owner, c, path->bound());
	}

	void Graphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		NVGcontext *c = context();
		Size sz = rect.size();
		nvgBeginPath(c);
		nvgRect(c, rect.p0.x, rect.p0.y, sz.w, sz.h);
		nvgFillPaint(c, nvgImagePattern(c, rect.p0.x, rect.p0.y, sz.w, sz.h, 0, bitmap->texture(owner), opacity));
		nvgFill(c);
	}

	void Graphics::text(Str *text, Font *font, Brush *style, Rect rect) {}

	void Graphics::draw(Text *text, Brush *style, Point origin) {}

#endif

}
