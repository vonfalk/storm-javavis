#pragma once

#include "Skia.h"
#include "Gui/TextMgr.h"
#include "Text.h"

#ifdef GUI_GTK
#if HAS_SK_PARAGRAPH

namespace gui {

	class SkiaParText : public TextMgr {
	public:
		// Create.
		SkiaParText();

		// Destroy.
		~SkiaParText();

		// Create a font.
		virtual Resource createFont(const Font *font);

		// Create a text layout.
		virtual Resource createLayout(const Text *text);

		// Update the layout border of a layout.
		virtual bool updateBorder(void *layout, Size border);

		// Add a new effect to the layout. Note: "graphics" may be null if we are not currently rendering.
		virtual EffectResult addEffect(void *layout, const Text::Effect &effect, Str *text, MAYBE(Graphics *) graphics);

		// Get the actual size of the layout.
		virtual Size size(void *layout);

		// Get information about each line of the formatted text.
		virtual Array<TextLine *> *lineInfo(void *layout, Text *text);

		// Get a set of rectangles that cover a range of characters.
		virtual Array<Rect> *boundsOf(void *layout, Text *text, Str::Iter begin, Str::Iter end);

	private:
		// Font collection.
		sk_sp<skia::textlayout::FontCollection> fontCollection;
	};

	class ParText : public SkiaText {
	public:
		ParText(std::unique_ptr<skia::textlayout::Paragraph> layout, size_t utf8Bytes);

		// The layout.
		std::unique_ptr<skia::textlayout::Paragraph> layout;

		// Total number of UTF-8 bytes in the string. Needed to update the text style.
		size_t utf8Bytes;

		// Draw the text.
		virtual void draw(SkCanvas &to, const SkPaint &paint, Point origin);
	};
}

#endif
#endif
