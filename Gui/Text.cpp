#include "stdafx.h"
#include "Text.h"
#include "RenderMgr.h"
#include "Core/Convert.h"
#include "Core/Utf.h"
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

	Text::Text(Str *text, Font *font) : myText(text), myFont(font), effects(null) {
		float maxFloat = std::numeric_limits<float>::max();
		init(text, font, Size(maxFloat, maxFloat));
	}

	Text::Text(Str *text, Font *font, Size size) : myText(text), myFont(font), effects(null) {
		init(text, font, size);
	}

	Text::~Text() {
		destroy();
	}

	void Text::color(Str::Iter begin, Str::Iter end, SolidBrush *color) {
		this->color(begin, end, color->color());
	}

	Text::Effect::Effect(Nat from, Nat to, Color color) : from(from), to(to), color(color) {}


#ifdef GUI_WIN32

	void Text::init(Str *text, Font *font, Size size) {
		RenderMgr *mgr = renderMgr(engine());
		HRESULT r = mgr->dWrite()->CreateTextLayout(text->c_str(),
													text->peekLength(),
													font->textFormat(),
													size.w,
													size.h,
													&l);
		if (FAILED(r)) {
			WARNING("Failed to create layout: " << ::toS(r));
		}
	}

	void Text::destroy() {
		::release(l);
	}

	Size Text::size() {
		DWRITE_TEXT_METRICS metrics;
		l->GetMetrics(&metrics);
		return Size(metrics.width + borderExtra, metrics.height + borderExtra);
	}

	Size Text::layoutBorder() {
		return Size(l->GetMaxWidth(), l->GetMaxHeight());
	}

	void Text::layoutBorder(Size s) {
		l->SetMaxWidth(s.w);
		l->SetMaxHeight(s.h);
	}

	IDWriteTextLayout *Text::layout(Painter *owner) {
		if (effects) {
			TODO(L"FIXME");
			// for (Nat i = 0; i < effects->count(); i++) {
			// 	Effect &e = effects->at(i);
			// 	DWRITE_TEXT_RANGE range = { e.from, e.to };
			// 	ID2D1SolidColorBrush *brush;
			// 	owner->renderTarget()->CreateSolidColorBrush(dx(e.color), &brush);
			// 	l->SetDrawingEffect(brush, range);
			// 	brush->Release();
			// }

			// No need to update the effects any other time, the solid color brush is device independent.
			effects = null;
		}

		return l;
	}

	Array<TextLine *> *Text::lineInfo() {
		UINT32 numLines = 0;
		l->GetLineMetrics(NULL, 0, &numLines);

		DWRITE_LINE_METRICS *lines = new DWRITE_LINE_METRICS[numLines];
		l->GetLineMetrics(lines, numLines, &numLines);

		Array<TextLine *> *result = new (this) Array<TextLine *>();
		Str::Iter start = myText->begin();

		Float yPos = 0;

		for (Nat i = 0; i < numLines; i++) {
			// TODO: I don't know if 'length' is the number of characters or codepoints.
			DWRITE_LINE_METRICS &l = lines[i];
			Str::Iter pos = start;
			for (Nat j = 0; j < l.length - l.newlineLength; j++)
				++pos;

			*result << new (this) TextLine(yPos + l.baseline, myText->substr(start, pos));
			yPos += l.height;

			for (Nat j = 0; j < l.newlineLength; j++)
				++pos;
			start = pos;
		}

		delete []lines;

		return result;
	}

	void Text::color(Str::Iter begin, Str::Iter end, Color color) {
		if (!effects)
			effects = new (this) Array<Effect>();

		*effects << Effect(begin.offset(), end.offset(), color);
	}

	static inline Rect computeRect(IDWriteTextLayout *l, Nat from, Nat to) {
		Rect total;
		DWRITE_HIT_TEST_METRICS metrics;
		for (Nat i = from; i < to; i += metrics.length) {
			FLOAT x, y;
			l->HitTestTextPosition(i, false, &x, &y, &metrics);

			Rect ch(Point(metrics.left, metrics.top), Size(metrics.width, metrics.height));
			if (i == from)
				total = ch;
			else
				total = total.include(ch);
		}

		return total;
	}

	Array<Rect> *Text::boundsOf(Str::Iter begin, Str::Iter end) {
		Array<Rect> *result = new (this) Array<Rect>();

		UINT32 numLines = 0;
		l->GetLineMetrics(NULL, 0, &numLines);

		DWRITE_LINE_METRICS *lines = new DWRITE_LINE_METRICS[numLines];
		l->GetLineMetrics(lines, numLines, &numLines);

		Nat lineBegin = 0;
		for (Nat line = 0; line < numLines; line++) {
			Nat lineEnd = lineBegin + lines[line].length;

			if (lineEnd > begin.offset())
				*result << computeRect(l, max(lineBegin, begin.offset()), min(lineEnd, end.offset()));

			// Done?
			if (end.offset() <= lineEnd)
				break;

			lineBegin = lineEnd;
		}

		delete [] lines;

		return result;
	}

#endif
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
