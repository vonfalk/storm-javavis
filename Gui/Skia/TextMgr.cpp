#include "stdafx.h"
#include "TextMgr.h"
#include "Core/Convert.h"
#include "Core/Utf.h"
#include <limits>

#ifdef GUI_GTK

namespace gui {

	using skia::textlayout::FontCollection;
	using skia::textlayout::TextStyle;
	using skia::textlayout::TextDecoration;
	using skia::textlayout::ParagraphStyle;
	using skia::textlayout::Paragraph;
	using skia::textlayout::ParagraphBuilder;

	SkiaText::SkiaText() {
		fontCollection = sk_make_sp<FontCollection>();
		fontCollection->setDefaultFontManager(SkFontMgr::RefDefault());
	}

	SkiaText::~SkiaText() {}

	static void destroyParaStyle(void *fs) {
		ParagraphStyle *style = (ParagraphStyle *)fs;
		delete style;
	}

	SkiaText::Resource SkiaText::createFont(const Font *font) {
		TextStyle style;
		// TODO: We might need to be explicit with fallbacks here.
		style.setFontFamilies({SkString(font->name()->utf8_str())});
		SkFontStyle props(font->weight(),
						SkFontStyle::kNormal_Width,
						font->italic() ? SkFontStyle::kItalic_Slant : SkFontStyle::kUpright_Slant);
		style.setFontStyle(props);
		style.setFontSize(font->height()); // Might be pixels and not pt. Not documented in SkParagraph...

		TextDecoration decoration = skia::textlayout::kNoDecoration;
		if (font->underline())
			decoration = TextDecoration(decoration | skia::textlayout::kUnderline);
		if (font->strikeOut())
			decoration = TextDecoration(decoration | skia::textlayout::kLineThrough);
		style.setDecoration(decoration);
		style.setDecorationMode(skia::textlayout::kThrough);
		style.setDecorationStyle(skia::textlayout::kSolid);

		ParagraphStyle *para = new ParagraphStyle();
		para->setTextStyle(style);

		return SkiaText::Resource(para, &destroyParaStyle);
	}

	static void destroyParagraph(void *para) {
		ParData *p = (ParData *)para;
		delete p;
	}

	SkiaText::Resource SkiaText::createLayout(const Text *text) {
		ParagraphStyle *style = (ParagraphStyle *)text->font()->backendFont();

		// This is what the samples are doing, even though it seems we should not access the type directly ourselves.
		auto builder = ParagraphBuilder::make(*style, fontCollection);

		// TODO: Handle tabs!

		// TODO: Apply color effects! That means we need to sort the sub-ranges and figure out if they overlap.

		const char *s = text->text()->utf8_str();
		builder->addText(s, strlen(s));

		ParData *data = new ParData();

		data->layout = builder->Build();
		data->layout->layout(text->layoutBorder().w);
		data->utf8Bytes = strlen(s); // TODO: Avoid re-computing the length.

		return SkiaText::Resource(data, &destroyParagraph);
	}

	bool SkiaText::updateBorder(void *layout, Size s) {
		ParData *par = (ParData *)layout;
		par->layout->layout(s.w);

		return true;
	}

	SkiaText::EffectResult SkiaText::addEffect(void *layout, const Text::Effect &effect, Str *myText, Graphics *) {
		// TODO!
		return eApplied;
	}

	Size SkiaText::size(void *layout) {
		ParData *par = (ParData *)layout;

		return Size(par->layout->getMaxWidth(), par->layout->getHeight());
	}

	Array<TextLine *> *SkiaText::lineInfo(void *layout, Text *t) {
		// TODO!
		return null;
	}

	Array<Rect> *SkiaText::boundsOf(void *layout, Text *stormText, Str::Iter begin, Str::Iter end) {
		// TODO!
		return null;
	}

}

#endif
