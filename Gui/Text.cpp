#include "stdafx.h"
#include "Text.h"
#include "RenderMgr.h"
#include <limits>

namespace gui {

	// Small extra space added around the border to prevent unneeded wrapping.
	static const float borderExtra = 0.001f;

	Text::Text(Str *text, Font *font) {
		float maxFloat = std::numeric_limits<float>::max();
		init(text, font, Size(maxFloat, maxFloat));
	}

	Text::Text(Str *text, Font *font, Size size) {
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

#endif
#ifdef GUI_GTK

	void Text::init(Str *text, Font *font, Size size) {
		RenderMgr *mgr = renderMgr(engine());
		l = pango_layout_new(mgr->dummyContext());

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

#endif

}
