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
	using skia::textlayout::LineMetrics;
	using skia::textlayout::TextBox;

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

	struct Effect {
		Color color;
		Nat start;
		Nat end;
		Nat index;

		bool operator <(const Effect &o) const {
			if (start != o.start)
				return start < o.start;
			if (end != o.end)
				return end < o.end;
			return index < o.index;
		}
	};

	// Get a set of non-overlapping effects. We ensure that all intervals are sorted from start to end as well.
	static std::vector<Effect> linearizeEffects(Array<Text::Effect> *input) {
		if (input == null)
			return std::vector<Effect>();

		std::vector<Effect> effects(input->count());
		for (Nat i = 0; i < input->count(); i++) {
			const Text::Effect &e = input->at(i);
			Effect n = {
				e.color,
				e.from,
				e.to,
				i
			};
			effects.push_back(n);
		}

		// Sort them.
		std::sort(effects.begin(), effects.end());

		// Now, we can iterate through them and merge any overlaps.
		// TODO

		return effects;
	}

	void put(std::unique_ptr<ParagraphBuilder> &builder, Str *str, Nat from, Nat to, size_t &length) {
		if (from == to)
			return;

		Str *sub = str->substr(str->posIter(from), str->posIter(to));
		const char *utf8 = sub->utf8_str();

		size_t len = strlen(utf8);
		builder->addText(utf8, len);
		length += len;
	}

	SkiaText::Resource SkiaText::createLayout(const Text *text) {
		ParagraphStyle *style = (ParagraphStyle *)text->font()->backendFont();
		TextStyle textStyle = style->getTextStyle();

		// TODO: Handle tabs!

		std::vector<Effect> effects = linearizeEffects(text->peekEffects());
		PVAR(effects.size());

		std::unique_ptr<ParagraphBuilder> builder = ParagraphBuilder::make(*style, fontCollection);
		Str *str = text->text();
		size_t totalLength = 0;
		Nat offset = 0;

		for (size_t i = 0; i < effects.size(); i++) {
			Effect &e = effects[i];
			put(builder, str, offset, e.start, totalLength);

			SkPaint paint;
			paint.setAntiAlias(true);
			paint.setColor(skia(e.color));
			textStyle.setForegroundColor(paint);
			builder->pushStyle(textStyle);
			put(builder, str, e.start, e.end, totalLength);
			builder->pop();

			offset = e.end;
		}

		put(builder, str, offset, str->peekLength(), totalLength);

		ParData *data = new ParData();

		data->layout = builder->Build();
		data->layout->layout(text->layoutBorder().w);
		data->utf8Bytes = totalLength;

		return SkiaText::Resource(data, &destroyParagraph);
	}

	bool SkiaText::updateBorder(void *layout, Size s) {
		ParData *par = (ParData *)layout;
		par->layout->layout(s.w);

		return true;
	}

	SkiaText::EffectResult SkiaText::addEffect(void *, const Text::Effect &, Str *, Graphics *) {
		// Skia does not support changing the text effect on a part of the text after it is created.
		return eReCreate;
	}

	Size SkiaText::size(void *layout) {
		ParData *par = (ParData *)layout;

		return Size(par->layout->getMaxWidth(), par->layout->getHeight());
	}

	struct Hint {
		size_t utf8;
		Str::Iter str;
		Str::Iter prev;

		Hint(Str *s) : utf8(0), str(s->begin()), prev(s->begin()) {}
	};

	static void step(Hint &hint, size_t to) {
		Str::Iter end;
		byte buffer[utf8::maxBytes];

		while (hint.utf8 < to) {
			if (hint.str == end)
				break;

			nat count = 0;
			utf8::encode(hint.str.v().codepoint(), buffer, &count);
			hint.utf8 += count;
			hint.prev = hint.str;
			++hint.str;
		}
	}

	static Str *extract(Str *from, size_t utf8Start, size_t utf8End, Hint &hint) {
		if (utf8Start < hint.utf8)
			hint = Hint(from);

		step(hint, utf8Start);
		Str::Iter start = hint.str;

		step(hint, utf8End);
		Str::Iter end = hint.str;

		// Even though the implementation hints at not including newlines in the range, it
		// does... We don't want to do that, so we explicitly remove the trailing newline.
		if (hint.prev.v() == Char('\n'))
			end = hint.prev;

		return from->substr(start, end);
	}

	Array<TextLine *> *SkiaText::lineInfo(void *layout, Text *t) {
		ParData *par = (ParData *)layout;

		Str *text = t->text();
		Array<TextLine *> *result = new (t) Array<TextLine *>();
		Hint hint(text);
		std::vector<LineMetrics> metrics;

		result->reserve(metrics.size());

		par->layout->getLineMetrics(metrics);
		for (size_t i = 0; i < metrics.size(); i++) {
			const LineMetrics &m = metrics[i];
			Str *content = extract(text, m.fStartIndex, m.fEndIndex, hint);
			*result << new (t) TextLine(m.fBaseline, content);
		}

		return result;
	}

	Array<Rect> *SkiaText::boundsOf(void *layout, Text *stormText, Str::Iter begin, Str::Iter end) {
		ParData *par = (ParData *)layout;

		// unsigned startIndex = 0;
		// unsigned endIndex = 0;
		// unsigned id = 0;
		// Str *text = stormText->text();
		// for (Str::Iter i = text->begin(), last = text->end(); i != last; ++i, id++) {
		// 	if (i == begin) {
		// 		startIndex = id;
		// 	}
		// 	if (i == end) {
		// 		endIndex = id;
		// 		break;
		// 	}
		// }

		// Interestingly enough, the implementation in SkParagraph seems to accept UTF16 indices,
		// not glyph indices as indicated in the header... The upside is that it is easier for us.
		unsigned startIndex = begin.offset();
		unsigned endIndex = end.offset();

		Array<Rect> *result = new (stormText) Array<Rect>();
		std::vector<TextBox> boxes = par->layout->getRectsForRange(startIndex, endIndex,
																skia::textlayout::RectHeightStyle::kMax,
																skia::textlayout::RectWidthStyle::kTight);

		result->reserve(boxes.size());
		for (size_t i = 0; i < boxes.size(); i++) {
			const TextBox &box = boxes[i];
			*result << Rect(box.rect.fLeft, box.rect.fTop, box.rect.fRight, box.rect.fBottom);
		}

		return result;
	}

}

#endif
