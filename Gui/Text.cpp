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
		return Size(metrics.width, metrics.height);
	}

	Size Text::layoutBorder() {
		return Size(l->GetMaxWidth() - borderExtra, l->GetMaxHeight() - borderExtra);
	}

	void Text::layoutBorder(Size s) {
		l->SetMaxWidth(s.w + borderExtra);
		l->SetMaxHeight(s.h + borderExtra);
	}

#endif
#ifdef GUI_GTK

	void Text::init(Str *text, Font *font, Size size) {
		TODO(L"We need some way of acquiring a cairo_t representative of the screen.");
	}

	void Text::destroy() {}

	Size Text::size() {
		return Size();
	}

	Size Text::layoutBorder() {
		return Size();
	}

	void Text::layoutBorder(Size s) {}

#endif

}
