#include "stdafx.h"
#include "Text.h"
#include "RenderMgr.h"
#include "Core/Convert.h"
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

	Text::Text(Str *text, Font *font) : myText(text), myFont(font) {
		float maxFloat = std::numeric_limits<float>::max();
		init(text, font, Size(maxFloat, maxFloat));
	}

	Text::Text(Str *text, Font *font, Size size) : myText(text), myFont(font) {
		init(text, font, size);
	}

	Text::~Text() {
		destroy();
	}

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

#endif
#ifdef GUI_GTK

	void Text::init(Str *text, Font *font, Size size) {
		RenderMgr *mgr = renderMgr(engine());
		l = pango_layout_new(mgr->pango());

		pango_layout_set_wrap(l, PANGO_WRAP_WORD_CHAR);
		pango_layout_set_font_description(l, font->desc());
		layoutBorder(size);
		pango_layout_set_text(l, text->utf8_str(), -1);
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

#endif

}
