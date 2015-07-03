#include "stdafx.h"
#include "Text.h"
#include "RenderMgr.h"

namespace stormgui {

	Text::Text(Par<Str> text, Par<Font> font, Size size) : s(size) {
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

}
