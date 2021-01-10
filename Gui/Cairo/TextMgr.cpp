#include "stdafx.h"
#include "TextMgr.h"
#include "Core/Convert.h"
#include "Core/Utf.h"
#include <limits>

#ifdef GUI_GTK

namespace gui {

	CairoText::CairoText() {
		// Note: This should be executed from the rendering thread. Otherwise, we get a shared font
		// map with the UI thread, which could lead to strange crashes.
		context = pango_font_map_create_context(pango_cairo_font_map_get_default());
	}

	CairoText::~CairoText() {
		g_object_unref(context);
	}

	CairoText::Resource CairoText::createFont(const Font *font) {
		// We're reusing the fonts already in the Font object.
		// In the future, we might want to create our own copies to avoid shared data.
		// It does, however, seem like the font description is a "dumb" container as
		// it does not require access to a context of any sort when creating it.
		return CairoText::Resource();
	}

	static void freeLayout(void *layout) {
		g_object_unref(layout);
	}

	static PangoAttrList *layout_attrs(PangoLayout *l) {
		PangoAttrList *attrs = pango_layout_get_attributes(l);
		if (!attrs) {
			attrs = pango_attr_list_new();
			pango_layout_set_attributes(l, attrs);
			pango_attr_list_unref(attrs);
		}
		return attrs;
	}

	CairoText::Resource CairoText::createLayout(const Text *text) {
		PangoLayout *l = pango_layout_new(context);

		Font *font = text->font();

		pango_layout_set_wrap(l, PANGO_WRAP_WORD_CHAR);
		pango_layout_set_font_description(l, font->desc());
		updateBorder(l, text->layoutBorder());
		const char *utf8 = text->text()->utf8_str();
		int utf8Bytes = strlen(utf8);
		pango_layout_set_text(l, utf8, utf8Bytes);

		Float tabWidth = text->font()->tabWidth();
		if (tabWidth > 0) {
			PangoTabArray *array = pango_tab_array_new(1, false);
			pango_tab_array_set_tab(array, 0, PANGO_TAB_LEFT, toPango(tabWidth));
			pango_layout_set_tabs(l, array);
			pango_tab_array_free(array);
		}

		if (font->underline()) {
			PangoAttrList *attrs = layout_attrs(l);
			PangoAttribute *attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
			attr->start_index = 0;
			attr->end_index = utf8Bytes;
			pango_attr_list_change(attrs, attr);
		}

		if (font->strikeOut()) {
			PangoAttrList *attrs = layout_attrs(l);
			PangoAttribute *attr = pango_attr_strikethrough_new(TRUE);
			attr->start_index = 0;
			attr->end_index = utf8Bytes;
			pango_attr_list_change(attrs, attr);
		}

		return Resource(l, &freeLayout);
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

	bool CairoText::updateBorder(void *layout, Size s) {
		PangoLayout *l = (PangoLayout *)layout;

		int w = -1;
		int h = -1;
		if (s.w < std::numeric_limits<float>::max())
			w = toPango(s.w);
		if (s.h < std::numeric_limits<float>::max())
			h = toPango(s.h);
		pango_layout_set_width(l, w);
		pango_layout_set_height(l, h);

		return true;
	}

	CairoText::EffectResult CairoText::addEffect(void *layout, const Text::Effect &effect, Str *myText, Graphics *) {
		PangoLayout *l = (PangoLayout *)layout;

		PangoAttrList *attrs = layout_attrs(l);

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
			pango_attr_list_change(attrs, alpha);
		}

		// Tell Pango to re-compute the layout since we changed the formatting. Otherwise we are not
		// able to modify the text formatting after we have rendered it once.
		pango_layout_context_changed(l);

		return eApplied;
	}

	Size CairoText::size(void *layout) {
		PangoLayout *l = (PangoLayout *)layout;

		int w, h;
		pango_layout_get_size(l, &w, &h);
		return Size(fromPango(w), fromPango(h));
	}

	Array<TextLine *> *CairoText::lineInfo(void *layout, Text *t) {
		PangoLayout *l = (PangoLayout *)layout;

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

	Array<Rect> *CairoText::boundsOf(void *layout, Text *stormText, Str::Iter begin, Str::Iter end) {
		PangoLayout *l = (PangoLayout *)layout;

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
