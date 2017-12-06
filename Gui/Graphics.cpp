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

	void Graphics::text(Str *text, Font *font, Brush *brush, Rect rect) {
		ID2D1Brush *b = brush->brush(owner, rect);
		info.target()->DrawText(text->c_str(), text->peekLength(), font->textFormat(), dx(rect), b);
	}

	void Graphics::draw(Text *text, Brush *brush, Point origin) {
		info.target()->DrawTextLayout(dx(origin), text->layout(), brush->brush(owner, Rect(origin, text->size())));
	}

	void Graphics::prepare() {}

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
		state.layer = Layer::none;
	}

	void Graphics::push(Float opacity) {
		oldStates->push(state);
		state.layer = Layer::group;
		state.opacity = opacity;
		cairo_push_group(info.device());
	}

	void Graphics::push(Rect clip) {
		oldStates->push(state);
		state.layer = Layer::save;
		cairo_save(info.device());

		Size sz = clip.size();
		cairo_rectangle(info.device(), clip.p0.x, clip.p0.y, sz.w, sz.h);
		cairo_clip(info.device());
	}

	void Graphics::push(Rect clip, Float opacity) {
		oldStates->push(state);
		state.layer = Layer::group;
		state.opacity = opacity;
		cairo_push_group(info.device());

		Size sz = clip.size();
		cairo_rectangle(info.device(), clip.p0.x, clip.p0.y, sz.w, sz.h);
		cairo_clip(info.device());
	}

	Bool Graphics::pop() {
		switch (state.layer.kind()) {
		case Layer::none:
			break;
		case Layer::group:
			cairo_pop_group_to_source(info.device());
			cairo_paint_with_alpha(info.device(), state.opacity);
			break;
		case Layer::save:
			cairo_restore(info.device());
			break;
		}

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
		cairo_matrix_t m = cairoMultiply(cairo(tfm), oldStates->last().transform());
		state.transform(m);
		cairo_set_matrix(info.device(), &m);
	}

	void Graphics::lineWidth(Float w) {
		// Note: Line size is affected by transforms at the time of stroking. Is this what we desire?
		state.lineWidth = oldStates->last().lineWidth * w;
		cairo_set_line_width(info.device(), state.lineWidth);
	}

	void Graphics::prepare() {
		cairo_matrix_t tfm = state.transform();
		cairo_set_matrix(info.device(), &tfm);
		cairo_set_line_width(info.device(), state.lineWidth);
	}

	void Graphics::Layer::release() {
		v = null;
	}

	/**
	 * Draw stuff.
	 */

	void Graphics::line(Point from, Point to, Brush *style) {
		cairo_new_path(info.device());
		cairo_move_to(info.device(), from.x, from.y);
		cairo_line_to(info.device(), to.x, to.y);

		style->setSource(info.device(), Rect(from, to).normalized());

		cairo_stroke(info.device());
	}

	void Graphics::draw(Rect rect, Brush *style) {
		Size sz = rect.size();
		cairo_rectangle(info.device(), rect.p0.x, rect.p0.y, sz.w, sz.h);
		style->setSource(info.device(), rect);
		cairo_stroke(info.device());
	}

	static void rectangle(const RenderInfo &info, const Rect &rect) {
		Size sz = rect.size();
		cairo_rectangle(info.device(), rect.p0.x, rect.p0.y, sz.w, sz.h);
	}

	static void rounded_corner(const RenderInfo &info, Point center, Size scale, double from, double to) {
		cairo_save(info.device());
		cairo_translate(info.device(), center.x, center.y);
		cairo_scale(info.device(), scale.w, scale.h);
		cairo_arc(info.device(), 0, 0, 1, from, to);
		cairo_restore(info.device());
	}

	static void rounded_rect(const RenderInfo &to, Rect rect, Size edges) {
		const double quarter = M_PI / 2;

		cairo_new_path(to.device());
		rounded_corner(to, Point(rect.p1.x - edges.w, rect.p0.y + edges.h), edges, -quarter, 0);
		rounded_corner(to, Point(rect.p1.x - edges.w, rect.p1.y - edges.h), edges, 0, quarter);
		rounded_corner(to, Point(rect.p0.x + edges.w, rect.p1.y - edges.h), edges, quarter, 2*quarter);
		rounded_corner(to, Point(rect.p0.x + edges.w, rect.p0.y + edges.h), edges, 2*quarter, 3*quarter);
		cairo_close_path(to.device());
	}

	void Graphics::draw(Rect rect, Size edges, Brush *style) {
		rounded_rect(info, rect, edges);

		style->setSource(info.device(), rect);
		cairo_stroke(info.device());
	}

	static void cairo_oval(const RenderInfo &to, Rect rect) {
		cairo_save(to.device());

		Point center = rect.center();
		cairo_translate(to.device(), center.x, center.y);
		Size size = rect.size();
		cairo_scale(to.device(), size.w / 2, size.h / 2);

		cairo_arc(to.device(), 0, 0, 1, 0, 2*M_PI);

		cairo_restore(to.device());
	}

	void Graphics::oval(Rect rect, Brush *style) {
		cairo_oval(info, rect);

		style->setSource(info.device(), rect);
		cairo_stroke(info.device());
	}

	void Graphics::draw(Path *path, Brush *style) {
		path->draw(info.device());
		style->setSource(info.device(), path->bound());
		cairo_stroke(info.device());
	}

	void Graphics::fill(Rect rect, Brush *style) {
		rectangle(info, rect);
		style->setSource(info.device(), rect);
		cairo_fill(info.device());
	}

	void Graphics::fill(Rect rect, Size edges, Brush *style) {
		rounded_rect(info, rect, edges);

		style->setSource(info.device(), rect);
		cairo_fill(info.device());
	}

	void Graphics::fill(Brush *style) {
		cairo_matrix_t tfm;
		cairo_matrix_init_identity(&tfm);
		cairo_set_matrix(info.device(), &tfm);

		style->setSource(info.device(), Rect(Point(0, 0), size()));
		cairo_paint(info.device());

		tfm = state.transform();
		cairo_set_matrix(info.device(), &tfm);
	}

	void Graphics::fillOval(Rect rect, Brush *style) {
		cairo_oval(info, rect);

		style->setSource(info.device(), rect);
		cairo_fill(info.device());
	}

	void Graphics::fill(Path *path, Brush *style) {
		path->draw(info.device());
		style->setSource(info.device(), path->bound());
		cairo_fill(info.device());
	}

	void Graphics::draw(Bitmap *bitmap) {}

	void Graphics::draw(Bitmap *bitmap, Point topLeft) {}

	void Graphics::draw(Bitmap *bitmap, Point topLeft, Float opacity) {}

	void Graphics::draw(Bitmap *bitmap, Rect rect) {}

	void Graphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {}

	void Graphics::text(Str *text, Font *font, Brush *style, Rect rect) {
		// Note: It would be good to not have to create the layout all the time.
		PangoLayout *layout = pango_cairo_create_layout(info.device());

		pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
		pango_layout_set_font_description(layout, font->desc());
		pango_layout_set_width(layout, toPango(rect.size().w));
		pango_layout_set_height(layout, toPango(rect.size().h));
		pango_layout_set_text(layout, text->utf8_str(), -1);

		style->setSource(info.device(), rect);

		cairo_move_to(info.device(), rect.p0.x, rect.p0.y);
		pango_cairo_show_layout(info.device(), layout);

		g_object_unref(layout);
	}

	void Graphics::draw(Text *text, Brush *brush, Point origin) {}

#endif

}
