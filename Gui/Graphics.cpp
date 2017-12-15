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

		for (nat i = 0; i < layerHistory->count; i++)
			layerHistory->v[i] = 0;
	}

	Graphics::~Graphics() {
		for (nat i = 0; i < layers->count(); i++)
			layers->at(i).release();
	}

	void Graphics::updateTarget(RenderInfo info) {
		this->info = info;

		// Remove any layers.
		for (nat i = 0; i < layers->count(); i++)
			layers->at(i).release();
		layers->clear();
	}

	Size Graphics::size() {
		return info.size;
	}

	void Graphics::destroyed() {
		info = RenderInfo();
		owner = null;
	}

	/**
	 * State management.
	 */

	void Graphics::beforeRender() {
		minFreeLayers = layers->count();
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

#ifdef GUI_WIN32

	void Graphics::reset() {
		// Do any PopLayer calls required.
		while (oldStates->count() > 1) {
			if (state.layer.v) {
				layers->push(state.layer.v);
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

	void Graphics::push() {
		oldStates->push(state);
		// state.layer = Layer::none;
	}

	void Graphics::push(Float opacity) {
		oldStates->push(state);
		// state.layer = Layer::group;
		// state.opacity = opacity;
		// cairo_push_group(info.device());
	}

	void Graphics::push(Rect clip) {
		oldStates->push(state);
		// state.layer = Layer::save;
		// cairo_save(info.device());

		// Size sz = clip.size();
		// cairo_rectangle(info.device(), clip.p0.x, clip.p0.y, sz.w, sz.h);
		// cairo_clip(info.device());
	}

	void Graphics::push(Rect clip, Float opacity) {
		oldStates->push(state);
		// state.layer = Layer::group;
		// state.opacity = opacity;
		// cairo_push_group(info.device());

		// Size sz = clip.size();
		// cairo_rectangle(info.device(), clip.p0.x, clip.p0.y, sz.w, sz.h);
		// cairo_clip(info.device());
	}

	Bool Graphics::pop() {
		// switch (state.layer.kind()) {
		// case Layer::none:
		// 	break;
		// case Layer::group:
		// 	cairo_pop_group_to_source(info.device());
		// 	cairo_paint_with_alpha(info.device(), state.opacity);
		// 	break;
		// case Layer::save:
		// 	cairo_restore(info.device());
		// 	break;
		// }

		state = oldStates->last();
		prepare();

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
		// // Note: Line size is affected by transforms at the time of stroking. Is this what we desire?
		// state.lineWidth = oldStates->last().lineWidth * w;
		// cairo_set_line_width(info.device(), state.lineWidth);
	}

	void Graphics::prepare() {
		NVGcontext *c = info.context()->nvg;
		nvgSetTransform(c, state.transform());
		nvgStrokeWidth(c, state.lineWidth);
	}

	void Graphics::Layer::release() {
		v = null;
	}

	NVGcontext *Graphics::context() {
		GlContext *c = info.context();
		assert(c);

		// Activate the context in case there was a thread switch somewhere since the last paint
		// operation.
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
		style->stroke(c, Rect(from, to).normalized(), state.opacity);
	}

	void Graphics::draw(Rect rect, Brush *style) {
		NVGcontext *c = context();
		nvgBeginPath(c);
		Size sz = rect.size();
		nvgRect(c, rect.p0.x, rect.p0.y, sz.w, sz.h);
		style->stroke(c, rect, state.opacity);
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
		style->stroke(c, rect, state.opacity);
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
		style->stroke(c, rect, state.opacity);
	}

	void Graphics::draw(Path *path, Brush *style) {
		NVGcontext *c = context();
		path->draw(c);
		style->stroke(c, path->bound(), state.opacity);
	}

	void Graphics::fill(Rect rect, Brush *style) {
		NVGcontext *c = context();
		nvgBeginPath(c);
		Size sz = rect.size();
		nvgRect(c, rect.p0.x, rect.p0.y, sz.w, sz.h);
		style->fill(c, rect, state.opacity);
	}

	void Graphics::fill(Rect rect, Size edges, Brush *style) {
		NVGcontext *c = context();
		nvgBeginPath(c);
		roundedRect(c, rect, edges);
		style->fill(c, rect, state.opacity);
	}

	void Graphics::fill(Brush *style) {
		NVGcontext *c = context();
		nvgResetTransform(c);
		Size sz = size();
		nvgBeginPath(c);
		nvgRect(c, 0, 0, sz.w + 1.0f, sz.h + 1.0f);
		style->fill(c, Rect(Point(), sz), state.opacity);
		// Go back to the previous transform.
		nvgSetTransform(c, state.transform());
	}

	void Graphics::fillOval(Rect rect, Brush *style) {
		NVGcontext *c = context();
		nvgBeginPath(c);
		drawOval(c, rect);
		style->fill(c, rect, state.opacity);
	}

	void Graphics::fill(Path *path, Brush *style) {
		NVGcontext *c = context();
		path->draw(c);
		style->fill(c, path->bound(), state.opacity);
	}

	void Graphics::draw(Bitmap *bitmap) {}

	void Graphics::draw(Bitmap *bitmap, Point topLeft) {}

	void Graphics::draw(Bitmap *bitmap, Point topLeft, Float opacity) {}

	void Graphics::draw(Bitmap *bitmap, Rect rect) {}

	void Graphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {}

	void Graphics::text(Str *text, Font *font, Brush *style, Rect rect) {}

	void Graphics::draw(Text *text, Brush *style, Point origin) {}

#endif

}
