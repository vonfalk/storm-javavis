#include "stdafx.h"
#include "PangoText.h"

#ifdef GUI_GTK

namespace gui {

	/**
	 * Various classes for storing text rendering operations.
	 */

	/**
	 * Render a text blob.
	 */
	class TextOp : public PangoText::Operation {
	public:
		TextOp(sk_sp<SkTextBlob> text) : text(text) {}

		sk_sp<SkTextBlob> text;

		void draw(SkCanvas &to, const SkPaint &paint, Point origin) {
			to.drawTextBlob(text, origin.x, origin.y, paint);
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

		// Produced operations.
		std::vector<std::unique_ptr<PangoText::Operation>> operations;

		// TextBlobBuilder to concatenate as many runs as possible.
		SkTextBlobBuilder builder;

		// Push an operation.
		void push(PangoText::Operation *op) {
			operations.push_back(std::unique_ptr<PangoText::Operation>(op));
		}

		// Flush runs to builder.
		void flush() {
			// Note: Returns nullptr if empty:
			sk_sp<SkTextBlob> blob = builder.make();
			if (blob)
				push(new TextOp(blob));
		}
	};

	struct SkRendererClass {
		PangoRendererClass parent_class;
	};

	G_DEFINE_TYPE(SkRenderer, sk_renderer, PANGO_TYPE_RENDERER);

	static void sk_begin(PangoRenderer *renderer) {
		SkRenderer *render = (SkRenderer *)renderer;
		render->builder.make(); // Clears the builder.
		render->operations.clear();
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
		SkPangoFont &skFont = sk->cache->get(font);

		// Needed. It seems we cannot mix matrix-based and position based runs. Should be fine,
		// Pango merges things for us.
		sk->flush();

		PangoColor *color = pango_renderer_get_color(renderer, PANGO_RENDER_PART_FOREGROUND);
		PVAR(color);

		guint16 alpha = pango_renderer_get_alpha(renderer, PANGO_RENDER_PART_FOREGROUND);
		PVAR(alpha);

		const PangoMatrix *matrix = pango_renderer_get_matrix(renderer);
		PVAR(matrix);

		// Count the non-empty glyphs.
		int glyphCount = 0;
		for (int i = 0; i < glyphs->num_glyphs; i++) {
			if (!ignoreGlyph(glyphs->glyphs[i]))
				glyphCount++;
		}

		if (skFont.transform) {
			// Respect the transform. We can not represent "slants", so we base it on the Y coordinate.
			SkRSXform base = SkRSXform::Make(skFont.transform->yy, skFont.transform->xy, 0, 0);

			const SkTextBlobBuilder::RunBuffer &buffer = sk->builder.allocRunRSXform(skFont.skia, glyphCount);
			int dest = 0;
			for (int i = 0; i < glyphs->num_glyphs; i++) {
				PangoGlyphInfo &glyph = glyphs->glyphs[i];
				if (!ignoreGlyph(glyph)) {
					base.fTx = fromPango(x + glyph.geometry.x_offset);
					base.fTy = fromPango(y + glyph.geometry.y_offset);

					buffer.glyphs[dest] = glyph.glyph;
					buffer.xforms()[dest] = base;
					dest++;
				}
				x += glyph.geometry.width;
			}
		} else {
			// No transform to worry about.
			const SkTextBlobBuilder::RunBuffer &buffer = sk->builder.allocRunPos(skFont.skia, glyphCount);
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

		// TODO: We need to draw underlines, overlines and strike-through as well it seems.
	}

	static void sk_draw_rectangle(PangoRenderer *renderer, PangoRenderPart part, int x, int y, int width, int height) {
		PLN(L"Rectangle");
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

	static void sk_renderer_class_init(SkRendererClass *klass) {
		PangoRendererClass *render = PANGO_RENDERER_CLASS(klass);

		render->draw_glyphs = sk_draw_glyphs;
		render->draw_rectangle = sk_draw_rectangle;
		render->draw_trapezoid = sk_draw_trapezoid;
		render->draw_error_underline = sk_draw_error;
		render->draw_shape = sk_draw_shape;
		render->begin = sk_begin;
		render->end = sk_end;

		// If we implement this one, we can get the actual text if we want to.
		// render->draw_glyph_item = sk_draw_glyph_item;

		// Note: Documentation says "draw_glyph" must be implemented, but PangoCairoRenderer only
		// implements draw_glyphs and draw_glyph_item, so we're probably fine.
	}

	static void sk_renderer_init(SkRenderer *renderer) {
		renderer->cache = null;
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
			operations = std::move(renderer->operations);
			g_object_unref(renderer);

			valid = true;
		}

		for (size_t i = 0; i < operations.size(); i++) {
			operations[i]->draw(to, paint, origin);
		}
	}

}

#endif
