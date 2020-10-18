#include "stdafx.h"
#include "WindowGraphics.h"
#include "Painter.h"
#include "Brush.h"
#include "Text.h"
#include "Path.h"
#include "Bitmap.h"

namespace gui {

	WindowGraphics::WindowGraphics(RenderInfo info, Painter *p) : info(info), owner(p) {
		oldStates = new (this) Array<State>();
		layers = new (this) Array<Layer>();

		state = State();
		oldStates->push(state);
		layerHistory = runtime::allocArray<Nat>(engine(), &natArrayType, layerHistoryCount);

		for (Nat i = 0; i < layerHistory->count; i++)
			layerHistory->v[i] = 0;
	}

	WindowGraphics::~WindowGraphics() {
		for (Nat i = 0; i < layers->count(); i++)
			layers->at(i).release();
	}

	void WindowGraphics::updateTarget(RenderInfo info) {
		this->info = info;

		// Remove any layers.
		for (Nat i = 0; i < layers->count(); i++)
			layers->at(i).release();
		layers->clear();
	}

	Size WindowGraphics::size() {
		return info.size / info.scale;
	}

	void WindowGraphics::destroyed() {
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

	void WindowGraphics::beforeRender() {
		// Keep track of free layers so we can remove them if we have too many.
		minFreeLayers = layers->count();

		// Update the clip region and scale of the root state.
		state = State();
		state.scale(info.scale);
		state.clip = Rect(Point(), info.size);
		oldStates->last() = state;

		// Set up the backend.
		prepare();

		PLN(L"start_frame(dev);");
	}

	void WindowGraphics::afterRender() {
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

		PLN("end_frame(dev);\n");
	}


#ifdef GUI_WIN32

	void WindowGraphics::reset() {
		// Do any PopLayer calls required.
		while (oldStates->count() > 1) {
			if (state.layer) {
				if (state.layer == Layer::dummy()) {
					info.target()->PopAxisAlignedClip();
				} else {
					layers->push(state.layer);
					info.target()->PopLayer();
				}
			}
			state = oldStates->last();
			oldStates->pop();
		}

		state = oldStates->at(0);
	}

	void WindowGraphics::prepare() {
		info.target()->SetTransform(*state.transform());
	}

	Bool WindowGraphics::pop() {
		if (state.layer) {
			if (state.layer == Layer::dummy()) {
				info.target()->PopAxisAlignedClip();
			} else {
				layers->push(state.layer);
				info.target()->PopLayer();
			}
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

	ID2D1Layer *WindowGraphics::layer() {
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

	void WindowGraphics::push() {
		oldStates->push(state);
		state.layer = Layer();
	}

	void WindowGraphics::push(Float opacity) {
		// Optimization for cases when opacity >= 1.
		if (opacity >= 1.0f) {
			push();
			return;
		}

		oldStates->push(state);
		state.layer = layer();

		D2D1_LAYER_PARAMETERS p = defaultParameters();
		p.opacity = opacity;
		info.target()->PushLayer(&p, state.layer.v);
	}

	void WindowGraphics::push(Rect clip) {
		oldStates->push(state);
		state.layer = Layer::dummy();

		info.target()->PushAxisAlignedClip(dx(clip), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	}

	void WindowGraphics::push(Rect clip, Float opacity) {
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
		info.target()->PushLayer(&p, state.layer.v);
	}

	void WindowGraphics::transform(Transform *tfm) {
		*state.transform() = dxMultiply(dx(tfm), *oldStates->last().transform());
		info.target()->SetTransform(*state.transform());
	}

	void WindowGraphics::lineWidth(Float w) {
		// TODO: How is the width of lines affected by scaling etc.? How do we want it to behave?
		// TODO: PDF does not set the line width in relation to previous states. Maybe we should not either?
		state.lineWidth = oldStates->last().lineWidth * w;
	}

	/**
	 * Draw stuff.
	 */

	void WindowGraphics::line(Point from, Point to, Brush *style) {
		info.target()->DrawLine(dx(from), dx(to), style->brush(owner), state.lineWidth);
	}

	void WindowGraphics::draw(Rect rect, Brush *style) {
		info.target()->DrawRectangle(dx(rect), style->brush(owner), state.lineWidth);
	}

	void WindowGraphics::draw(Rect rect, Size edges, Brush *style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		info.target()->DrawRoundedRectangle(r, style->brush(owner), state.lineWidth);
	}

	void WindowGraphics::oval(Rect rect, Brush *style) {
		Size s = rect.size() / 2;
		D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		info.target()->DrawEllipse(e, style->brush(owner), state.lineWidth);
	}

	void WindowGraphics::draw(Path *path, Brush *style) {
		info.target()->DrawGeometry(path->geometry(), style->brush(owner), state.lineWidth);
	}

	void WindowGraphics::fill(Rect rect, Brush *style) {
		info.target()->FillRectangle(dx(rect), style->brush(owner));
	}

	void WindowGraphics::fill(Rect rect, Size edges, Brush *style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		info.target()->FillRoundedRectangle(r, style->brush(owner));
	}

	void WindowGraphics::fill(Brush *style) {
		Rect s(Point(), size());
		info.target()->SetTransform(D2D1::Matrix3x2F::Identity());
		info.target()->FillRectangle(dx(s), style->brush(owner));
		info.target()->SetTransform(*state.transform());
	}

	void WindowGraphics::fillOval(Rect rect, Brush *style) {
		Size s = rect.size() / 2;
		D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		info.target()->FillEllipse(e, style->brush(owner));
	}

	void WindowGraphics::fill(Path *path, Brush *style) {
		info.target()->FillGeometry(path->geometry(), style->brush(owner));
	}

	void WindowGraphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		info.target()->DrawBitmap(bitmap->bitmap(owner), &dx(rect), opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
	}

	void WindowGraphics::draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) {
		info.target()->DrawBitmap(bitmap->bitmap(owner), &dx(dest), opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &dx(src));
	}

	void WindowGraphics::text(Str *text, Font *font, Brush *style, Rect rect) {
		ID2D1Brush *b = style->brush(owner);
		info.target()->DrawText(text->c_str(), text->peekLength(), font->textFormat(), dx(rect), b);
	}

	void WindowGraphics::draw(Text *text, Brush *style, Point origin) {
			info.target()->DrawTextLayout(dx(origin), text->layout(owner), style->brush(owner));
	}

	void WindowGraphics::Layer::release() {
		v->Release();
		v = null;
	}

#endif
#ifdef GUI_GTK

	/**
	 * State management.
	 */

	void WindowGraphics::reset() {
		// Clear any remaining states from the stack.
		while (pop())
			;
	}

	void WindowGraphics::push() {
		oldStates->push(state);
		state.layer = LayerKind::none;
	}

	void WindowGraphics::push(Float opacity) {
		oldStates->push(state);
		state.layer = LayerKind::group;
		state.opacity = opacity;
		printf("cairo_push_group(...);\n");
		cairo_push_group(info.target());
	}

	void WindowGraphics::push(Rect clip) {
		oldStates->push(state);
		state.layer = LayerKind::save;
		printf("cairo_save(...);\n");
		cairo_save(info.target());

		Size sz = clip.size();
		cairo_rectangle(info.target(), clip.p0.x, clip.p0.y, sz.w, sz.h);
		printf("cairo_clip(...);\n");
		cairo_clip(info.target());
	}

	void WindowGraphics::push(Rect clip, Float opacity) {
		oldStates->push(state);
		state.layer = LayerKind::group;
		state.opacity = opacity;
		printf("cairo_push_group(...);\n");
		cairo_push_group(info.target());

		Size sz = clip.size();
		printf("cairo_clip(...);\n");
		cairo_rectangle(info.target(), clip.p0.x, clip.p0.y, sz.w, sz.h);
		cairo_clip(info.target());
	}

	Bool WindowGraphics::pop() {
		switch (state.layer.v) {
		case LayerKind::none:
			break;
		case LayerKind::group:
			PLN("cairo_pop_to_source(...);\ncairo_paint_with_alpha(..., %f);");
			cairo_pop_group_to_source(info.target());
			cairo_paint_with_alpha(info.target(), state.opacity);
			break;
		case LayerKind::save:
			PLN("cairo_restore(...);");
			cairo_restore(info.target());
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

	void WindowGraphics::transform(Transform *tfm) {
		cairo_matrix_t m = cairoMultiply(cairo(tfm), oldStates->last().transform());
		state.transform(m);
		printf("cairo_set_matrix(...);\n");
		// cairo_set_matrix(info.target(), &m);
	}

	void WindowGraphics::lineWidth(Float w) {
		// TODO: How is the width of lines affected by scaling etc.? How do we want it to behave?
		state.lineWidth = oldStates->last().lineWidth * w;
		printf("cairo_set_line_width(..., %f);\n", state.lineWidth);
		// cairo_set_line_width(info.target(), state.lineWidth);
	}

	void WindowGraphics::prepare() {
		cairo_matrix_t tfm = state.transform();
		// cairo_set_matrix(info.target(), &tfm);
		printf("cairo_set_line_width(..., %f); // in prepare\n", state.lineWidth);
		// cairo_set_line_width(info.target(), state.lineWidth);
	}

	void WindowGraphics::Layer::release() {
		v = LayerKind::none;
	}

	/**
	 * Draw stuff.
	 */

	void WindowGraphics::line(Point from, Point to, Brush *style) {
		PLN(L"cairo_new_path(dev);");
		cairo_new_path(info.target());
		PLN(L"cairo_move_to(dev, " << from.x << ", " << from.y << L");");
		cairo_move_to(info.target(), from.x, from.y);
		PLN(L"cairo_line_to(dev, " << to.x << ", " << to.y << L");");
		cairo_line_to(info.target(), to.x, to.y);

		style->setSource(owner, info.target());

		PLN("cairo_stroke(dev);");
		cairo_stroke(info.target());
	}

	void WindowGraphics::draw(Rect rect, Brush *style) {
		Size sz = rect.size();
		cairo_rectangle(info.target(), rect.p0.x, rect.p0.y, sz.w, sz.h);
		style->setSource(owner, info.target());
		printf("cairo_stroke(...);\n");
		cairo_stroke(info.target());
	}

	static void rectangle(const RenderInfo &info, const Rect &rect) {
		Size sz = rect.size();
		cairo_rectangle(info.target(), rect.p0.x, rect.p0.y, sz.w, sz.h);
	}

	static void rounded_corner(const RenderInfo &info, Point center, Size scale, double from, double to) {
		cairo_save(info.target());
		cairo_translate(info.target(), center.x, center.y);
		cairo_scale(info.target(), scale.w, scale.h);
		cairo_arc(info.target(), 0, 0, 1, from, to);
		cairo_restore(info.target());
	}

	static void rounded_rect(const RenderInfo &to, Rect rect, Size edges) {
		const double quarter = M_PI / 2;

		cairo_new_path(to.target());
		rounded_corner(to, Point(rect.p1.x - edges.w, rect.p0.y + edges.h), edges, -quarter, 0);
		rounded_corner(to, Point(rect.p1.x - edges.w, rect.p1.y - edges.h), edges, 0, quarter);
		rounded_corner(to, Point(rect.p0.x + edges.w, rect.p1.y - edges.h), edges, quarter, 2*quarter);
		rounded_corner(to, Point(rect.p0.x + edges.w, rect.p0.y + edges.h), edges, 2*quarter, 3*quarter);
		cairo_close_path(to.target());
	}

	void WindowGraphics::draw(Rect rect, Size edges, Brush *style) {
		rounded_rect(info, rect, edges);

		style->setSource(owner, info.target());
		printf("cairo_stroke(...);\n");
		cairo_stroke(info.target());
	}

	static void cairo_oval(const RenderInfo &to, Rect rect) {
		cairo_save(to.target());

		Point center = rect.center();
		cairo_translate(to.target(), center.x, center.y);
		Size size = rect.size();
		cairo_scale(to.target(), size.w / 2, size.h / 2);

		cairo_new_path(to.target());
		cairo_arc(to.target(), 0, 0, 1, 0, 2*M_PI);

		cairo_restore(to.target());
	}

	void WindowGraphics::oval(Rect rect, Brush *style) {
		cairo_oval(info, rect);

		style->setSource(owner, info.target());
		printf("cairo_stroke(...);\n");
		cairo_stroke(info.target());
	}

	void WindowGraphics::draw(Path *path, Brush *style) {
		path->draw(info.target());
		style->setSource(owner, info.target());
		printf("cairo_stroke(...);\n");
		cairo_stroke(info.target());
	}

	void WindowGraphics::fill(Rect rect, Brush *style) {
		rectangle(info, rect);
		style->setSource(owner, info.target());
		printf("cairo_fill(...);\n");
		cairo_fill(info.target());
	}

	void WindowGraphics::fill(Rect rect, Size edges, Brush *style) {
		rounded_rect(info, rect, edges);

		style->setSource(owner, info.target());
		printf("cairo_fill(...);\n");
		cairo_fill(info.target());
	}

	void WindowGraphics::fill(Brush *style) {
		cairo_matrix_t tfm;
		cairo_matrix_init_identity(&tfm);
		// cairo_set_matrix(info.target(), &tfm);

		style->setSource(owner, info.target());
		printf("cairo_paint(...);\n");
		cairo_paint(info.target());

		tfm = state.transform();
		// cairo_set_matrix(info.target(), &tfm);
	}

	void WindowGraphics::fillOval(Rect rect, Brush *style) {
		cairo_oval(info, rect);

		style->setSource(owner, info.target());
		printf("cairo_fill(...);\n");
		cairo_fill(info.target());
	}

	void WindowGraphics::fill(Path *path, Brush *style) {
		path->draw(info.target());
		style->setSource(owner, info.target());
		printf("cairo_fill(...);\n");
		cairo_fill(info.target());
	}

	void WindowGraphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		printf("cairo_draw_bitmap(..., %f); // #1\n", opacity);

		cairo_pattern_t *pattern = cairo_pattern_create_for_surface(bitmap->get<cairo_surface_t>(owner));
		cairo_matrix_t tfm;
		Size original = bitmap->size();
		Size target = rect.size();
		cairo_matrix_init_scale(&tfm, original.w / target.w, original.h / target.h);
		cairo_matrix_translate(&tfm, -rect.p0.x, -rect.p0.y);
		// cairo_pattern_set_matrix(pattern, &tfm);
		cairo_set_source(info.target(), pattern);
		if (opacity < 1.0f)
			cairo_paint_with_alpha(info.target(), opacity);
		else
			cairo_paint(info.target());
		cairo_pattern_destroy(pattern);
	}

	void WindowGraphics::draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) {
		printf("cairo_draw_bitmap(...); // #2\n");

		cairo_save(info.target());

		rectangle(info, dest);
		cairo_clip(info.target());

		cairo_pattern_t *pattern = cairo_pattern_create_for_surface(bitmap->get<cairo_surface_t>(owner));
		cairo_matrix_t tfm;
		Size original = src.size();
		Size target = dest.size();
		cairo_matrix_init_translate(&tfm, src.p0.x, src.p0.y);
		cairo_matrix_scale(&tfm, original.w / target.w, original.h / target.h);
		cairo_matrix_translate(&tfm, -dest.p0.x, -dest.p0.y);
		// cairo_pattern_set_matrix(pattern, &tfm);
		cairo_set_source(info.target(), pattern);
		if (opacity < 1.0f)
			cairo_paint_with_alpha(info.target(), opacity);
		else
			cairo_paint(info.target());
		cairo_pattern_destroy(pattern);

		cairo_restore(info.target());
	}

	void WindowGraphics::text(Str *text, Font *font, Brush *style, Rect rect) {
		printf("pango_cairo_show_layout(...); // on-the-fly\n");

		// Note: It would be good to not have to create the layout all the time.
		PangoLayout *layout = pango_cairo_create_layout(info.target());

		pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
		pango_layout_set_font_description(layout, font->desc());
		// Account for rounding errors...
		pango_layout_set_width(layout, toPango(rect.size().w + 0.3f));
		pango_layout_set_height(layout, toPango(rect.size().h + 0.3f));
		pango_layout_set_text(layout, text->utf8_str(), -1);

		style->setSource(owner, info.target());

		cairo_move_to(info.target(), rect.p0.x, rect.p0.y);
		pango_cairo_show_layout(info.target(), layout);

		g_object_unref(layout);
	}

	void WindowGraphics::draw(Text *text, Brush *style, Point origin) {
		this->text(text->text(), text->font(), style, Rect(origin, Size(100, 100)));
		return;



		style->setSource(owner, info.target());

		PLN(L"cairo_move_to(dev, " << origin.x << L", " << origin.y << L");");
		PLN(L"pango_cairo_show_layout(dev, " << (void *)text->layout(owner) << L");");

		cairo_move_to(info.target(), origin.x, origin.y);
		pango_cairo_show_layout(info.target(), text->layout(owner));

		cairo_surface_flush(cairo_get_group_target(info.target()));
	}

#endif

}
