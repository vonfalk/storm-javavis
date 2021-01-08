#include "stdafx.h"
#include "PangoMgr.h"
#include "PangoText.h"
#include "Core/Utf.h"
#include "Core/Convert.h"

#ifdef GUI_GTK

#include <pango/pangofc-fontmap.h>

namespace gui {

	SkiaPangoMgr::SkiaPangoMgr() {
		// Note: This should be executed from the rendering thread. Otherwise, we get a shared font
		// map with the UI thread, which could lead to strange crashes.
		context = pango_font_map_create_context(pango_cairo_font_map_get_default());
	}

	SkiaPangoMgr::~SkiaPangoMgr() {
		g_object_unref(context);
	}

	SkiaPangoMgr::Resource SkiaPangoMgr::createFont(const Font *font) {
		// We're reusing the fonts already in the Font object.
		// In the future, we might want to create our own copies to avoid shared data.
		// It does, however, seem like the font description is a "dumb" container as
		// it does not require access to a context of any sort when creating it.
		return SkiaPangoMgr::Resource();
	}

	static void freeLayout(void *layout) {
		PangoText *text = (PangoText *)layout;
		delete text;
	}

	static void updateBorder(PangoLayout *l, Size s) {
		int w = -1;
		int h = -1;
		if (s.w < std::numeric_limits<float>::max())
			w = toPango(s.w);
		if (s.h < std::numeric_limits<float>::max())
			h = toPango(s.h);
		pango_layout_set_width(l, w);
		pango_layout_set_height(l, h);
	}

	SkiaPangoMgr::Resource SkiaPangoMgr::createLayout(const Text *text) {
		PangoLayout *l = pango_layout_new(context);

		pango_layout_set_wrap(l, PANGO_WRAP_WORD_CHAR);
		pango_layout_set_font_description(l, text->font()->desc());
		gui::updateBorder(l, text->layoutBorder());
		pango_layout_set_text(l, text->text()->utf8_str(), -1);

		Float tabWidth = text->font()->tabWidth();
		if (tabWidth > 0) {
			PangoTabArray *array = pango_tab_array_new(1, false);
			pango_tab_array_set_tab(array, 0, PANGO_TAB_LEFT, toPango(tabWidth));
			pango_layout_set_tabs(l, array);
			pango_tab_array_free(array);
		}

		return Resource(new PangoText(l, cache), &freeLayout);
	}

	static inline guint16 pangoColor(Float v) {
		return guint16(v * 65535);
	}

	static inline size_t byteOffset(Str::Iter storm, const char *pango, Str::Iter goal) {
		size_t offset = 0;
		while (storm != Str::Iter() && storm.offset() < goal.offset()) {
			// Advance each iterator one codepoint.
			++storm;

			// Skip the first utf-8 byte...
			offset++;
			// ...and skip any continuation bytes.
			while (utf8::isCont(pango[offset]))
				offset++;
		}

		return offset;
	}

	static inline bool inRange(int prev, int curr, int target) {
		if (curr == prev)
			return target == prev;
		else if (curr > prev)
			return target > prev && target <= curr;
		else
			return target >= curr && target < prev;
	}

	static inline Rect fromPango(const PangoRectangle &r) {
		return Rect(Point(fromPango(r.x), fromPango(r.y)), Size(fromPango(r.width), fromPango(r.height)));
	}

	bool SkiaPangoMgr::updateBorder(void *layout, Size s) {
		PangoText *t = (PangoText *)layout;
		gui::updateBorder(t->pango, s);
		t->invalidate();
		return true;
	}

	SkiaPangoMgr::EffectResult SkiaPangoMgr::addEffect(void *layout, const Text::Effect &effect, Str *myText, Graphics *) {
		PangoText *t = (PangoText *)layout;
		PangoLayout *l = t->pango;

		PangoAttrList *attrs = pango_layout_get_attributes(l);
		if (!attrs) {
			attrs = pango_attr_list_new();
			pango_layout_set_attributes(l, attrs);
			pango_attr_list_unref(attrs);
		}

		Str::Iter begin = myText->posIter(effect.from);
		Str::Iter end = myText->posIter(effect.to);

		const char *text = pango_layout_get_text(l);
		size_t beginBytes = byteOffset(myText->begin(), text, begin);
		size_t endBytes = beginBytes + byteOffset(begin, text + beginBytes, end);

		Color color = effect.color;
		PangoAttribute *attr = pango_attr_foreground_new(pangoColor(color.r), pangoColor(color.g), pangoColor(color.b));
		attr->start_index = beginBytes;
		attr->end_index = endBytes;
		pango_attr_list_change(attrs, attr);

		if (color.a < 1.0f) {
			PangoAttribute *alpha = pango_attr_foreground_alpha_new(pangoColor(color.a));
			alpha->start_index = beginBytes;
			alpha->end_index = endBytes;
			pango_attr_list_insert(attrs, alpha);
		}

		t->invalidate();
		return eApplied;
	}

	Size SkiaPangoMgr::size(void *layout) {
		PangoText *t = (PangoText *)layout;
		PangoLayout *l = t->pango;

		int w, h;
		pango_layout_get_size(l, &w, &h);
		return Size(fromPango(w), fromPango(h));
	}

	Array<TextLine *> *SkiaPangoMgr::lineInfo(void *layout, Text *t) {
		PangoText *pangoText = (PangoText *)layout;
		PangoLayout *l = pangoText->pango;

		Array<TextLine *> *result = new (t) Array<TextLine *>();
		const char *text = pango_layout_get_text(l);

		PangoLayoutIter *iter = pango_layout_get_iter(l);
		do {
			Float baseline = fromPango(pango_layout_iter_get_baseline(iter));

			PangoLayoutLine *line = pango_layout_iter_get_line_readonly(iter);
			int from = line->start_index;
			Str *z = new (t) Str(toWChar(t->engine(), text + from, line->length));
			*result << new (t) TextLine(baseline, z);
		} while (pango_layout_iter_next_line(iter));
		pango_layout_iter_free(iter);

		return result;
	}

	Array<Rect> *SkiaPangoMgr::boundsOf(void *layout, Text *stormText, Str::Iter begin, Str::Iter end) {
		PangoText *t = (PangoText *)layout;
		PangoLayout *l = t->pango;

		Array<Rect> *result = new (stormText) Array<Rect>();
		const char *text = pango_layout_get_text(l);
		size_t beginBytes = byteOffset(stormText->text()->begin(), text, begin);
		size_t endBytes = beginBytes + byteOffset(begin, text + beginBytes, end);

		if (beginBytes == endBytes)
			return result;

		PangoRectangle rect;

		PangoLayoutIter *iter = pango_layout_get_iter(l);
		int lastIndex = pango_layout_iter_get_index(iter);
		do {
			int index = pango_layout_iter_get_index(iter);
			if (inRange(lastIndex, index, int(beginBytes)))
				break;

			lastIndex = index;
		} while (pango_layout_iter_next_cluster(iter));

		pango_layout_iter_get_cluster_extents(iter, NULL, &rect);
		Rect lineBounds = fromPango(rect);
		int lineStart = pango_layout_iter_get_line_readonly(iter)->start_index;

		while (pango_layout_iter_next_cluster(iter)) {
			int index = pango_layout_iter_get_index(iter);
			if (inRange(lastIndex, index, int(endBytes)))
				break;
			lastIndex = index;

			pango_layout_iter_get_cluster_extents(iter, NULL, &rect);
			int currLine = pango_layout_iter_get_line_readonly(iter)->start_index;

			// New line?
			if (currLine != lineStart) {
				*result << lineBounds;
				lineBounds = fromPango(rect);
				lineStart = currLine;
			} else {
				lineBounds = lineBounds.include(fromPango(rect));
			}
		}

		*result << lineBounds;

		pango_layout_iter_free(iter);

		return result;
	}

}

#endif
