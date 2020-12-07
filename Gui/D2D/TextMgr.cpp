#include "stdafx.h"
#include "TextMgr.h"
#include "Exception.h"
#include "Graphics.h"
#include "Device.h"

#ifdef GUI_WIN32

namespace gui {

	static void check(HRESULT r, const wchar *msg) {
		if (FAILED(r)) {
			Engine &e = runtime::someEngine();
			Str *m = TO_S(e, msg << ::toS(r).c_str());
			throw new (e) GuiError(m);
		}
	}

	D2DText::D2DText() {
		DWRITE_FACTORY_TYPE type = DWRITE_FACTORY_TYPE_SHARED;
		HRESULT r = DWriteCreateFactory(type, __uuidof(IDWriteFactory), (IUnknown **)&factory.v);
		check(r, S("Failed to intitialize Direct Write: "));
	}

	static void destroyCOM(void *object) {
		if (object) {
			IUnknown *i = (IUnknown *)object;
			i->Release();
		}
	}

	D2DText::Resource D2DText::createFont(const Font *font) {
		IDWriteTextFormat *out = null;
		DWRITE_FONT_STYLE style = font->italic() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
		DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;
		HRESULT r = factory->CreateTextFormat(font->name()->c_str(),
											NULL,
											(DWRITE_FONT_WEIGHT)font->weight(),
											style,
											stretch,
											font->pxHeight(),
											L"", // locale, we use defaults. Was "en-us".
											&out);

		check(r, S("Failed to create a Direct Write font."));

		if (font->tabWidth() > 0)
			out->SetIncrementalTabStop(font->tabWidth());

		return Resource(out, &destroyCOM);
	}

	D2DText::Resource D2DText::createLayout(const Text *text) {
		Str *t = text->text();
		Size border = text->layoutBorder();
		IDWriteTextFormat *format = (IDWriteTextFormat *)text->font()->backendFont();
		IDWriteTextLayout *layout;
		HRESULT r = factory->CreateTextLayout(t->c_str(), t->peekLength(), format, border.w, border.h, &layout);
		check(r, S("Failed to create a text layout: "));

		return D2DText::Resource(layout, &destroyCOM);
	}

	bool D2DText::updateBorder(void *layout, Size border) {
		IDWriteTextLayout *l = (IDWriteTextLayout *)layout;
		l->SetMaxWidth(border.w);
		l->SetMaxHeight(border.h);
		return true;
	}

	D2DText::EffectResult D2DText::addEffect(void *layout, const Text::Effect &effect, Str *, Graphics *graphics) {
		// We need a render target to be able to apply effects...
		if (!graphics)
			return eWait;

		// It should be a D2DGraphics object.
		D2DGraphics *g = as<D2DGraphics>(graphics);
		if (!g)
			return eWait;

		IDWriteTextLayout *l = (IDWriteTextLayout *)layout;
		DWRITE_TEXT_RANGE range = { effect.from, effect.to };
		ID2D1SolidColorBrush *brush;
		g->getSurface().target()->CreateSolidColorBrush(dx(effect.color), &brush);
		l->SetDrawingEffect(brush, range);
		brush->Release();

		return eApplied;
	}

	Size D2DText::size(void *layout) {
		IDWriteTextLayout *l = (IDWriteTextLayout *)layout;
		DWRITE_TEXT_METRICS metrics;
		l->GetMetrics(&metrics);
		return Size(metrics.width, metrics.height);
	}

	Array<TextLine *> *D2DText::lineInfo(void *layout, Text *text) {
		IDWriteTextLayout *l = (IDWriteTextLayout *)layout;
		Str *myText = text->text();

		UINT32 numLines = 0;
		l->GetLineMetrics(NULL, 0, &numLines);

		DWRITE_LINE_METRICS *lines = new DWRITE_LINE_METRICS[numLines];
		l->GetLineMetrics(lines, numLines, &numLines);

		Array<TextLine *> *result = new (text) Array<TextLine *>();
		Str::Iter start = myText->begin();

		Float yPos = 0;

		for (Nat i = 0; i < numLines; i++) {
			// TODO: I don't know if 'length' is the number of characters or codepoints.
			DWRITE_LINE_METRICS &l = lines[i];
			Str::Iter pos = start;
			for (Nat j = 0; j < l.length - l.newlineLength; j++)
				++pos;

			*result << new (text) TextLine(yPos + l.baseline, myText->substr(start, pos));
			yPos += l.height;

			for (Nat j = 0; j < l.newlineLength; j++)
				++pos;
			start = pos;
		}

		delete []lines;

		return result;
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

	Array<Rect> *D2DText::boundsOf(void *layout, Text *text, Str::Iter begin, Str::Iter end) {
		IDWriteTextLayout *l = (IDWriteTextLayout *)layout;
		Str *myText = text->text();

		Array<Rect> *result = new (text) Array<Rect>();

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

}

#endif
