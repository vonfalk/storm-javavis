#include "stdafx.h"
#include "Text.h"
#include "RenderMgr.h"
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
		return mgr->size(layout) + Size(borderExtra, borderExtra);
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

		if (mgr->addEffect(layout, myEffects->last(), myText, null) == TextMgr::eApplied) {
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
				TextMgr::EffectResult r = mgr->addEffect(layout, myEffects->at(i), myText, g);
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

}
