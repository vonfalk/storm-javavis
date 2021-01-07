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

	static void sk_draw_glyphs(PangoRenderer *renderer, PangoFont *font, PangoGlyphString *glyphs, int x, int y) {
		SkRenderer *sk = (SkRenderer *)renderer;

		PangoFcFont *fcFont = PANGO_FC_FONT(font);
		PVAR(pango_fc_font_get_pattern(fcFont));

		// FcPatternGetMatrix
		// FcPatternGetInteger with FC_WIDTH, FC_WEIGHT, FC_SLANT, etc.

		PangoColor *color = pango_renderer_get_color(renderer, PANGO_RENDER_PART_FOREGROUND);
		PVAR(color);

		guint16 alpha = pango_renderer_get_alpha(renderer, PANGO_RENDER_PART_FOREGROUND);
		PVAR(alpha);

		const PangoMatrix *matrix = pango_renderer_get_matrix(renderer);
		// TODO: Take "matrix" into account!

		hb_font_t *hbFont = pango_font_get_hb_font(font);
		hb_face_t *hbFace = hb_font_get_face(hbFont);
		hb_blob_t *hbBlob = hb_face_reference_blob(hbFace);

		// int xscale = 0, yscale = 0;
		// unsigned int xppem = 0, yppem = 0;
		// hb_font_get_scale(hbFont, &xscale, &yscale);
		// PVAR(xscale); PVAR(yscale);
		// PVAR(hb_font_get_ptem(hbFont));
		// hb_font_get_ppem(hbFont, &xppem, &yppem);
		// PVAR(xppem); PVAR(yppem);

		// TODO: If we use MakeWithoutCopy, and make sure to have a reference to the hb_blob
		// somewhere, we can avoid copying fonts.
		unsigned int size = 0;
		const char *fontData = hb_blob_get_data(hbBlob, &size);
		sk_sp<SkData> data = SkData::MakeWithCopy(fontData, size);

		// Unref the blob. Otherwise we leak memory.
		hb_blob_destroy(hbBlob);

		// TODO: Cache the typeface to avoid loading it all the time.
		sk_sp<SkTypeface> skTypeface = SkTypeface::MakeFromData(data);
		PLN(L"Done!");

		// It would be nice to not have to allocate a FontDescription...
		PangoFontDescription *description = pango_font_describe_with_absolute_size(font);
		float fontSize = fromPango(pango_font_description_get_size(description));
		PVAR(pango_font_description_get_family(description));
		pango_font_description_free(description);

		PVAR(skTypeface->getUnitsPerEm());

		SkFont skFont(skTypeface, fontSize);

		// TODO: We can set "edging" on the font.

		const SkTextBlobBuilder::RunBuffer &buffer = sk->builder.allocRunPos(skFont, glyphs->num_glyphs);

		// TODO: If the glyph is missing, we probably want to draw a "missing glyph" image or something.

		for (int i = 0; i < glyphs->num_glyphs; i++) {
			PangoGlyphInfo &glyph = glyphs->glyphs[i];

			if (glyph.glyph == PANGO_GLYPH_EMPTY) {
				// TODO: Ignore this glyph.
			} else if (glyph.glyph & PANGO_GLYPH_UNKNOWN_FLAG) {
				glyph.glyph &= ~PANGO_GLYPH_UNKNOWN_FLAG;
				PLN(L"Unknown glyph: " << glyph.glyph);
				// TODO: Draw an "unknown glyph" box?
			}

			SkPoint pos;
			pos.fX = fromPango(x + glyph.geometry.x_offset);
			pos.fY = fromPango(y + glyph.geometry.y_offset);
			x += glyph.geometry.width;

			buffer.glyphs[i] = glyph.glyph;
			buffer.points()[i] = pos;
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

	static void sk_renderer_init(SkRenderer *) {
		// No initialization required.
	}

	SkRenderer *sk_renderer_new() {
		return (SkRenderer *)g_object_new(sk_renderer_get_type(), NULL);
	}


	/**
	 * The wrapper against the rest of the system.
	 */

	PangoText::PangoText(PangoLayout *layout) : pango(layout), valid(false) {}

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
