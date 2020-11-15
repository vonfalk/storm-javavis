#include "stdafx.h"
#include "Graphics.h"
#include "Painter.h"
#include "Brush.h"
#include "Text.h"
#include "Path.h"
#include "Bitmap.h"

namespace gui {

	CairoGraphics::CairoGraphics(CairoSurface &surface) : surface(surface) {
		oldStates = new (this) Array<State>();

		state = State();
		oldStates->push(state);
	}

	CairoGraphics::~CairoGraphics() {}

#ifdef GUI_GTK

	Size CairoGraphics::size() {
		return info.size / info.scale;
	}

	void CairoGraphics::updateTarget(RenderInfo info) {
		this->info = info;
	}

	Size CairoGraphics::size() {
		return info.size / info.scale;
	}

	void CairoGraphics::destroyed() {
		info = RenderInfo();
	}

	/**
	 * State management.
	 */

	void CairoGraphics::beforeRender(Color bgColor) {
		// Make sure the cairo context is in a reasonable state.
		cairo_surface_mark_dirty(surface->surface);

		// Clear with the BG color.
		cairo_set_source_rgba(surface->cairo, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		cairo_set_operator(surface->cairo, CAIRO_OPERATOR_SOURCE);
		cairo_paint(surface->cairo);

		// Set defaults, just in case.
		cairo_set_operator(surface->cairo, CAIRO_OPERATOR_OVER);
		cairo_set_fill_rule(surface->cairo, CAIRO_FILL_RULE_EVEN_ODD);

		// Update the clip region and scale of the root state.
		state = State();
		state.scale(info.scale);
		state.clip = Rect(Point(), info.size);
		oldStates->last() = state;

		// Set up the backend.
		prepare();
	}

	bool CairoGraphics::afterRender() {
		// Make sure all layers are returned to the stack.
		reset();

		cairo_surface_flush(surface->surface);

		return true;
	}


	/**
	 * State management.
	 */

	void CairoGraphics::reset() {
		// Clear any remaining states from the stack.
		while (pop())
			;
	}

	void CairoGraphics::push() {
		oldStates->push(state);
		state.type = LayerKind::none;
	}

	void CairoGraphics::push(Float opacity) {
		oldStates->push(state);
		state.type = LayerKind::group;
		state.opacity = opacity;
		cairo_push_group(info.target());
	}

	void CairoGraphics::push(Rect clip) {
		oldStates->push(state);
		state.type = LayerKind::save;
		cairo_save(info.target());

		Size sz = clip.size();
		cairo_rectangle(info.target(), clip.p0.x, clip.p0.y, sz.w, sz.h);
		cairo_clip(info.target());
	}

	void CairoGraphics::push(Rect clip, Float opacity) {
		oldStates->push(state);
		state.type = LayerKind::group;
		state.opacity = opacity;
		cairo_push_group(info.target());

		Size sz = clip.size();
		cairo_rectangle(info.target(), clip.p0.x, clip.p0.y, sz.w, sz.h);
		cairo_clip(info.target());
	}

	Bool CairoGraphics::pop() {
		switch (state.type) {
		case LayerKind::none:
			break;
		case LayerKind::group:
			cairo_pop_group_to_source(info.target());
			cairo_paint_with_alpha(info.target(), state.opacity);
			break;
		case LayerKind::save:
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

	void CairoGraphics::transform(Transform *tfm) {
		cairo_matrix_t m = cairoMultiply(cairo(tfm), oldStates->last().transform());
		state.transform(m);
		cairo_set_matrix(info.target(), &m);
	}

	void CairoGraphics::lineWidth(Float w) {
		// TODO: How is the width of lines affected by scaling etc.? How do we want it to behave?
		state.lineWidth = oldStates->last().lineWidth *w;
		cairo_set_line_width(info.target(), state.lineWidth);
	}

	void CairoGraphics::prepare() {
		cairo_matrix_t tfm = state.transform();
		cairo_set_matrix(info.target(), &tfm);
		cairo_set_line_width(info.target(), state.lineWidth);
	}


	/**
	 * Draw stuff.
	 */

	void CairoGraphics::line(Point from, Point to, Brush *style) {
		cairo_new_path(info.target());
		cairo_move_to(info.target(), from.x, from.y);
		cairo_line_to(info.target(), to.x, to.y);

		style->setSource(owner, info.target());

		cairo_stroke(info.target());
	}

	void CairoGraphics::draw(Rect rect, Brush *style) {
		Size sz = rect.size();
		cairo_rectangle(info.target(), rect.p0.x, rect.p0.y, sz.w, sz.h);
		style->setSource(owner, info.target());
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

	void CairoGraphics::draw(Rect rect, Size edges, Brush *style) {
		rounded_rect(info, rect, edges);

		style->setSource(owner, info.target());
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

	void CairoGraphics::oval(Rect rect, Brush *style) {
		cairo_oval(info, rect);

		style->setSource(owner, info.target());
		cairo_stroke(info.target());
	}

	void CairoGraphics::draw(Path *path, Brush *style) {
		path->draw(info.target());
		style->setSource(owner, info.target());
		cairo_stroke(info.target());
	}

	void CairoGraphics::fill(Rect rect, Brush *style) {
		rectangle(info, rect);
		style->setSource(owner, info.target());
		cairo_fill(info.target());
	}

	void CairoGraphics::fill(Rect rect, Size edges, Brush *style) {
		rounded_rect(info, rect, edges);

		style->setSource(owner, info.target());
		cairo_fill(info.target());
	}

	void CairoGraphics::fill(Brush *style) {
		cairo_matrix_t tfm;
		cairo_matrix_init_identity(&tfm);
		cairo_set_matrix(info.target(), &tfm);

		style->setSource(owner, info.target());
		cairo_paint(info.target());

		tfm = state.transform();
		cairo_set_matrix(info.target(), &tfm);
	}

	void CairoGraphics::fillOval(Rect rect, Brush *style) {
		cairo_oval(info, rect);

		style->setSource(owner, info.target());
		cairo_fill(info.target());
	}

	void CairoGraphics::fill(Path *path, Brush *style) {
		path->draw(info.target());
		style->setSource(owner, info.target());
		cairo_fill(info.target());
	}

	void CairoGraphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		cairo_pattern_t *pattern = cairo_pattern_create_for_surface(bitmap->get<cairo_surface_t>(owner));
		cairo_matrix_t tfm;
		Size original = bitmap->size();
		Size target = rect.size();
		cairo_matrix_init_scale(&tfm, original.w / target.w, original.h / target.h);
		cairo_matrix_translate(&tfm, -rect.p0.x, -rect.p0.y);
		cairo_pattern_set_matrix(pattern, &tfm);
		cairo_set_source(info.target(), pattern);
		if (opacity < 1.0f)
			cairo_paint_with_alpha(info.target(), opacity);
		else
			cairo_paint(info.target());
		cairo_pattern_destroy(pattern);
	}

	void CairoGraphics::draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) {
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
		cairo_pattern_set_matrix(pattern, &tfm);
		cairo_set_source(info.target(), pattern);
		if (opacity < 1.0f)
			cairo_paint_with_alpha(info.target(), opacity);
		else
			cairo_paint(info.target());
		cairo_pattern_destroy(pattern);

		cairo_restore(info.target());
	}

	void CairoGraphics::text(Str *text, Font *font, Brush *style, Rect rect) {
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

	void CairoGraphics::draw(Text *text, Brush *style, Point origin) {
		style->setSource(owner, info.target());

		cairo_move_to(info.target(), origin.x, origin.y);
		pango_cairo_show_layout(info.target(), text->layout(owner));
	}

#else

	DEFINE_WINDOW_GRAPHICS_FNS(CairoGraphics)

#endif


}
