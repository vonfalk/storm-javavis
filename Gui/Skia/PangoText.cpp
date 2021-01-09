#include "stdafx.h"
#include "PangoText.h"

#ifdef GUI_GTK

namespace gui {

	/**
	 * The state implementation.
	 */

	PangoText::State::State() : hasColor(false), hasAlpha(false), matrix() {}

	PangoText::State::State(PangoRenderer *from, PangoRenderPart part) : hasColor(false), hasAlpha(false), matrix() {
		if (PangoColor *color = pango_renderer_get_color(from, part)) {
			this->color.fR = color->red / 65535.0f;
			this->color.fG = color->green / 65535.0f;
			this->color.fB = color->blue / 65535.0f;
			hasColor = true;
		}

		if (guint16 alpha = pango_renderer_get_alpha(from, part)) {
			color.fA = alpha / 65535.0f;
			hasAlpha = true;
		}

		if (const PangoMatrix *m = pango_renderer_get_matrix(from)) {
			matrix = SkMatrix::MakeAll(m->xx, m->xy, m->x0,
									m->yx, m->yy, m->y0,
									0, 0, 1);
		}
	}

	bool PangoText::State::operator ==(const State &o) const {
		if (hasColor != o.hasColor)
			return false;
		if (hasColor) {
			if (color.fR != o.color.fR ||
				color.fG != o.color.fG ||
				color.fB != o.color.fB)
				return false;
		}

		if (hasAlpha != o.hasAlpha)
			return false;
		if (hasAlpha) {
			if (color.fA != o.color.fA)
				return false;
		}

		return matrix == o.matrix;
	}


	/**
	 * Various classes for storing text rendering operations.
	 */

	/**
	 * Render a text blob.
	 */
	class TextOp : public PangoText::Operation {
	public:
		TextOp(sk_sp<SkTextBlob> text, std::vector<sk_sp<SkPangoFont>> fonts, PangoText::State state)
			: Operation(state), text(text), fonts(std::move(fonts)) {}

		~TextOp() {}

		sk_sp<SkTextBlob> text;

		// Fonts, so that the cache knows what we are using.
		std::vector<sk_sp<SkPangoFont>> fonts;

		void draw(SkCanvas &to, const SkPaint &paint, Point origin) {
			to.drawTextBlob(text, origin.x, origin.y, paint);
		}
	};

	/**
	 * Render a rectangle.
	 */
	class RectangleOp : public PangoText::Operation {
	public:
		RectangleOp(SkRect rect, PangoText::State state)
			: Operation(state), rect(rect) {}

		SkRect rect;

		void draw(SkCanvas &to, const SkPaint &paint, Point origin) {
			to.drawRect(rect.makeOffset(origin.x, origin.y), paint);
		}
	};


	/**
	 * Custom Pango renderer so that we can render the text into a representation that we then use
	 * for Skia. We do it this way so that we can cache many parts of the layout, and hopefully make
	 * rendering slightly faster in the end.
	 */

	struct SkRenderer {
		PangoRenderer parent;

		// Font cache.
		SkPangoFontCache *cache;

		// Separate struct for C++ types to ensure proper construction and destruction. GObject
		// likely calls malloc somewhere and is fine with that.
		struct Data {
			// Produced operations.
			std::vector<std::unique_ptr<PangoText::Operation>> operations;

			// TextBlobBuilder to concatenate as many runs as possible.
			SkTextBlobBuilder builder;

			// List containing all fonts in the builder. Kept so that the cache is aware of what we
			// are actually using.
			std::vector<sk_sp<SkPangoFont>> fonts;

			// Current state being built.
			PangoText::State state;
		};

		Data *data;

		// Push an operation.
		void push(PangoText::Operation *op) {
			data->operations.push_back(std::unique_ptr<PangoText::Operation>(op));
		}

		// Flush runs to builder.
		void flush() {
			// Note: Returns nullptr if empty:
			sk_sp<SkTextBlob> blob = data->builder.make();
			if (blob)
				push(new TextOp(blob, std::move(data->fonts), data->state));
			data->fonts = std::vector<sk_sp<SkPangoFont>>();
		}
	};

	struct SkRendererClass {
		PangoRendererClass parent_class;
	};

	G_DEFINE_TYPE(SkRenderer, sk_renderer, PANGO_TYPE_RENDERER);

	static void sk_begin(PangoRenderer *renderer) {
		SkRenderer *render = (SkRenderer *)renderer;
		render->data->builder.make(); // Clears the builder.
		render->data->state = PangoText::State();
		render->data->operations.clear();
		render->data->fonts = std::vector<sk_sp<SkPangoFont>>();
	}

	static void sk_end(PangoRenderer *renderer) {
		((SkRenderer *)renderer)->flush();
	}

	static bool ignoreGlyph(PangoGlyphInfo &glyph) {
		return glyph.glyph == PANGO_GLYPH_EMPTY
			|| (glyph.glyph & PANGO_GLYPH_UNKNOWN_FLAG) != 0;
	}

	static void sk_draw_glyphs(PangoRenderer *renderer, PangoFont *font, PangoGlyphString *glyphs, int x, int y) {
		SkRenderer *sk = (SkRenderer *)renderer;
		sk_sp<SkPangoFont> skFont = sk->cache->font(font);

		// If the state differs, start a new set of runs.
		PangoText::State state(renderer);
		if (state != sk->data->state) {
			sk->flush();
			sk->data->state = state;
		}

		// Count the non-empty glyphs.
		int glyphCount = 0;
		for (int i = 0; i < glyphs->num_glyphs; i++) {
			if (!ignoreGlyph(glyphs->glyphs[i]))
				glyphCount++;
		}

		if (glyphCount > 0) {
			// Remember the font. For caching purposes.
			sk->data->fonts.push_back(skFont);

			const SkTextBlobBuilder::RunBuffer &buffer = sk->data->builder.allocRunPos(skFont->font, glyphCount);
			int dest = 0;
			for (int i = 0; i < glyphs->num_glyphs; i++) {
				PangoGlyphInfo &glyph = glyphs->glyphs[i];
				if (!ignoreGlyph(glyph)) {
					SkPoint pos;
					pos.fX = fromPango(x + glyph.geometry.x_offset);
					pos.fY = fromPango(y + glyph.geometry.y_offset);

					buffer.glyphs[dest] = glyph.glyph;
					buffer.points()[dest] = pos;
					dest++;
				}
				x += glyph.geometry.width;
			}
		}

		// TODO: Draw "invalid" glyphs. Perhaps using rectangles.
	}

	static void sk_draw_rectangle(PangoRenderer *renderer, PangoRenderPart part, int x, int y, int width, int height) {
		SkRenderer *sk = (SkRenderer *)renderer;
		sk->flush();
		SkRect rect = SkRect::MakeXYWH(fromPango(x), fromPango(y), fromPango(width), fromPango(height));
		sk->push(new RectangleOp(rect, PangoText::State(renderer, part)));
	}

	static void sk_draw_trapezoid(PangoRenderer *renderer, PangoRenderPart part,
								double y1_, double x11, double x21,
								double y2, double x12, double x22) {
		PLN(L"Trapezoid");
	}

	static void sk_draw_error(PangoRenderer *renderer, int x, int y, int width, int height) {
		PLN(L"Error");
	}

	static void sk_draw_shape(PangoRenderer *renderer, PangoAttrShape *attr, int x, int y) {
		PLN(L"Shape");
	}

	static void sk_renderer_init(SkRenderer *renderer) {
		renderer->cache = null;
		renderer->data = new SkRenderer::Data();
	}

	static void sk_renderer_finalize(GObject *renderer) {
		SkRenderer *r = (SkRenderer *)renderer;
		delete r->data;
	}

	static void sk_renderer_class_init(SkRendererClass *klass) {
		PangoRendererClass *render = PANGO_RENDERER_CLASS(klass);

		render->draw_glyphs = sk_draw_glyphs;
		render->draw_rectangle = sk_draw_rectangle;
		render->draw_trapezoid = sk_draw_trapezoid;
		render->draw_error_underline = sk_draw_error;
		render->draw_shape = sk_draw_shape;
		render->begin = sk_begin;
		render->end = sk_end;

		GObjectClass *object = G_OBJECT_CLASS(klass);
		object->finalize = sk_renderer_finalize;

		// If we implement this one, we can get the actual text if we want to.
		// render->draw_glyph_item = sk_draw_glyph_item;

		// Note: Documentation says "draw_glyph" must be implemented, but PangoCairoRenderer only
		// implements draw_glyphs and draw_glyph_item, so we're probably fine.
	}

	SkRenderer *sk_renderer_new() {
		return (SkRenderer *)g_object_new(sk_renderer_get_type(), NULL);
	}


	/**
	 * The wrapper against the rest of the system.
	 */

	PangoText::PangoText(PangoLayout *layout, SkPangoFontCache &cache)
		: cache(cache), pango(layout), valid(false) {}

	PangoText::~PangoText() {
		g_object_unref(pango);
	}

	void PangoText::invalidate() {
		valid = false;
		operations.clear();
	}

	void PangoText::draw(SkCanvas &to, const SkPaint &paint, Point origin) {
		// If we have not done so already, update our state:
		if (!valid) {
			SkRenderer *renderer = sk_renderer_new();
			renderer->cache = &cache;
			pango_renderer_draw_layout(PANGO_RENDERER(renderer), pango, 0, 0);
			operations = std::move(renderer->data->operations);
			g_object_unref(renderer);

			valid = true;
		}

		SkM44 base = to.getLocalToDevice();

		for (size_t i = 0; i < operations.size(); i++) {
			SkPaint p = paint;
			State &s = operations[i]->state;
			if (s.hasColor && s.hasAlpha) {
				p.setColor4f(s.color);
			} else if (s.hasColor) {
				SkColor4f c = s.color;
				c.fA = p.getAlphaf();
				p.setColor4f(c);
			} else if (s.hasAlpha) {
				SkColor4f c = p.getColor4f();
				c.fA = s.color.fA;
				p.setColor4f(c);
			}

			// Note: This might not be correct. I have not found anywhere Pango returns matrices yet.
			SkM44 tfm(s.matrix);
			tfm.preConcat(base);
			to.setMatrix(tfm);
			operations[i]->draw(to, p, origin);
		}

		to.setMatrix(base);
	}

}

#endif
