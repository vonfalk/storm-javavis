#include "stdafx.h"
#include "Text.h"
#include "RenderMgr.h"
#include "Core/Convert.h"
#include "Core/Utf.h"
#include "TextMgr.h"
#include <limits>

namespace gui {

	TextLine::TextLine(Float baseline, Str *text) :
		baseline(baseline), text(text) {}

	void TextLine::toS(StrBuf *to) const {
		*to << S("{baseline=") << baseline << S(" text=")
			<< text << S("}");
	}

	// Small extra space added around the border to prevent unneeded wrapping.
	static const float borderExtra = 0.001f;

	Text::Text(Str *text, Font *font)
		: myText(text), myFont(font), myEffects(null), appliedEffects(0) {

		float maxFloat = std::numeric_limits<float>::max();
		myBorder = Size(maxFloat, maxFloat);
		init();
	}

	Text::Text(Str *text, Font *font, Size size)
		: myText(text), myFont(font), myBorder(size), myEffects(null), appliedEffects(0) {

		init();
	}

	void Text::init() {
		mgr = renderMgr(engine())->text();
		TextMgr::Resource r = mgr->createLayout(this);
		layout = r.data;
		cleanup = r.cleanup;
	}

	void Text::recreate() {
		if (cleanup)
			(*cleanup)(layout);
		TextMgr::Resource r = mgr->createLayout(this);
		layout = r.data;
		cleanup = r.cleanup;
	}

	Text::~Text() {
		if (layout && cleanup) {
			(*cleanup)(layout);
		}
	}

	Size Text::size() {
		return mgr->size(layout);
	}

	void Text::layoutBorder(Size border) {
		if (!mgr->updateBorder(layout, border)) {
			recreate();
		}
	}

	void Text::color(Str::Iter begin, Str::Iter end, SolidBrush *color) {
		this->color(begin, end, color->color());
	}

	void Text::color(Str::Iter begin, Str::Iter end, Color color) {
		if (!myEffects)
			myEffects = new (this) Array<Effect>();

		*myEffects << Effect(begin.offset(), end.offset(), color);

		if (mgr->addEffect(layout, myEffects->last(), null) == TextMgr::eApplied) {
			// We assume all calls behave the same way. So if we managed to apply this one, we're
			// done now.
			appliedEffects = myEffects->count();
		}
	}

	Array<TextLine *> *Text::lineInfo() {
		return mgr->lineInfo(layout, this);
	}

	Array<Rect> *Text::boundsOf(Str::Iter begin, Str::Iter end) {
		return mgr->boundsOf(layout, this, begin, end);
	}

	Text::Effect::Effect(Nat from, Nat to, Color color) : from(from), to(to), color(color) {}

	Array<Text::Effect> *Text::effects() const {
		if (!myEffects) {
			return new (this) Array<Effect>();
		} else {
			return new (this) Array<Effect>(*myEffects);
		}
	}

	void *Text::backendLayout(Graphics *g) {
		if (myEffects && appliedEffects < myEffects->count()) {

			for (Nat i = appliedEffects; i < myEffects->count(); i++) {
				TextMgr::EffectResult r = mgr->addEffect(layout, myEffects->at(i), g);
				if (r == TextMgr::eApplied) {
					// Nothing strange, continue.
					appliedEffects = i;
				} else if (r == TextMgr::eReCreate) {
					// Need to recreate the entire thing.
					recreate();
					break;
				} else if (r == TextMgr::eWait) {
					// Wrong context apparently. Wait for other draws.
					break;
				}
			}
		}

		// Now, we are ready!
		return layout;
	}


#ifdef GUI_GTK

	void Text::init(Str *text, Font *font, Size size) {
		RenderMgr *mgr = renderMgr(engine());
		l = pango_layout_new(mgr->pango());

		pango_layout_set_wrap(l, PANGO_WRAP_WORD_CHAR);
		pango_layout_set_font_description(l, font->desc());
		layoutBorder(size);
		pango_layout_set_text(l, text->utf8_str(), -1);

		Float tabWidth = font->tabWidth();
		if (tabWidth > 0) {
			PangoTabArray *array = pango_tab_array_new(1, false);
			pango_tab_array_set_tab(array, 0, PANGO_TAB_LEFT, toPango(font->tabWidth()));
			pango_layout_set_tabs(l, array);
			pango_tab_array_free(array);
		}
	}

	void Text::destroy() {
		g_object_unref(l);
	}

	Size Text::size() {
		int w, h;
		pango_layout_get_size(l, &w, &h);
		return Size(fromPango(w) + borderExtra,
					fromPango(h) + borderExtra);
	}

	Size Text::layoutBorder() {
		int w = pango_layout_get_width(l);
		int h = pango_layout_get_height(l);
		Size r(std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max());
		if (w >= 0)
			r.w = fromPango(w);
		if (h >= 0)
			r.h = fromPango(h);
		return r;
	}

	void Text::layoutBorder(Size s) {
		int w = -1;
		int h = -1;
		if (s.w < std::numeric_limits<float>::max())
			w = toPango(s.w);
		if (s.h < std::numeric_limits<float>::max())
			h = toPango(s.h);
		pango_layout_set_width(l, w);
		pango_layout_set_height(l, h);
	}

	Array<TextLine *> *Text::lineInfo() {
		Array<TextLine *> *result = new (this) Array<TextLine *>();
		const char *text = pango_layout_get_text(l);

		PangoLayoutIter *iter = pango_layout_get_iter(l);
		do {
			Float baseline = fromPango(pango_layout_iter_get_baseline(iter));

			PangoLayoutLine *line = pango_layout_iter_get_line_readonly(iter);
			int from = line->start_index;
			Str *t = new (this) Str(toWChar(engine(), text + from, line->length));
			*result << new (this) TextLine(baseline, t);
		} while (pango_layout_iter_next_line(iter));
		pango_layout_iter_free(iter);

		return result;
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

	void Text::color(Str::Iter begin, Str::Iter end, Color color) {
		PangoAttrList *attrs = pango_layout_get_attributes(l);
		if (!attrs) {
			attrs = pango_attr_list_new();
			pango_layout_set_attributes(l, attrs);
			pango_attr_list_unref(attrs);
		}

		const char *text = pango_layout_get_text(l);
		size_t beginBytes = byteOffset(myText->begin(), text, begin);
		size_t endBytes = beginBytes + byteOffset(begin, text + beginBytes, end);

		PangoAttribute *attr = pango_attr_foreground_new(pangoColor(color.r), pangoColor(color.g), pangoColor(color.b));
		attr->start_index = beginBytes;
		attr->end_index = endBytes;
		pango_attr_list_insert(attrs, attr);

		if (color.a < 1.0f) {
			PangoAttribute *alpha = pango_attr_foreground_alpha_new(pangoColor(color.a));
			alpha->start_index = beginBytes;
			alpha->end_index = endBytes;
			pango_attr_list_insert(attrs, alpha);
		}
	}

	static inline bool inRange(int prev, int curr, int target) {
		if (curr > prev)
			return target > prev && target <= curr;
		else
			return target >= curr && target < prev;
	}

	static inline Rect fromPango(const PangoRectangle &r) {
		return Rect(Point(fromPango(r.x), fromPango(r.y)), Size(fromPango(r.width), fromPango(r.height)));
	}

	Array<Rect> *Text::boundsOf(Str::Iter begin, Str::Iter end) {
		Array<Rect> *result = new (this) Array<Rect>();
		const char *text = pango_layout_get_text(l);
		size_t beginBytes = byteOffset(myText->begin(), text, begin);
		size_t endBytes = beginBytes + byteOffset(begin, text + beginBytes, end);

		if (beginBytes == endBytes)
			return result;

		PangoRectangle rect;

		PangoLayoutIter *iter = pango_layout_get_iter(l);
		int lastIndex = pango_layout_iter_get_index(iter);
		while (pango_layout_iter_next_cluster(iter)) {
			int index = pango_layout_iter_get_index(iter);
			if (inRange(lastIndex, index, int(beginBytes)))
				break;

			lastIndex = index;
		}

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

#endif

}
