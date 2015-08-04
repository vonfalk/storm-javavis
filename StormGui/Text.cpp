#include "stdafx.h"
#include "Text.h"
#include "RenderMgr.h"
#include <limits>

namespace stormgui {

	// Small extra space added around the border to prevent unneeded wrapping.
	static const float borderExtra = 0.001f;

	Text::Text(Par<Str> text, Par<Font> font) {
		float maxFloat = std::numeric_limits<float>::max();
		init(text, font, Size(maxFloat, maxFloat));
	}

	Text::Text(Par<Str> text, Par<Font> font, Size size) {
		init(text, font, size);
	}

	void Text::init(Par<Str> text, Par<Font> font, Size size) {
		Auto<RenderMgr> mgr = renderMgr(engine());
		HRESULT r = mgr->dWrite()->CreateTextLayout(text->v.c_str(),
													text->v.size(),
													font->textFormat(),
													size.w,
													size.h,
													&l);
		if (FAILED(r)) {
			WARNING("Failed to create layout: " << ::toS(r));
		}
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

}
