#include "stdafx.h"
#include "PangoFont.h"

#ifdef GUI_GTK

#include <pango/pangofc-font.h>

namespace gui {

	/**
	 * Typeface.
	 */

	SkPangoTypeface::SkPangoTypeface(hb_blob_t *blob) {
		unsigned int size;
		const char *data = hb_blob_get_data(blob, &size);
		this->data = SkData::MakeWithoutCopy(data, size);
		this->skia = SkTypeface::MakeFromData(this->data);
	}

	SkPangoTypeface::~SkPangoTypeface() {
		skia = sk_sp<SkTypeface>();
		data = sk_sp<SkData>();
		hb_blob_destroy(blob);
	}


	/**
	 * Font.
	 */

	static const FcMatrix *fcGetMatrix(FcPattern *pattern, const char *object) {
		FcMatrix *matrix = null;
		if (FcPatternGetMatrix(pattern, object, 0, &matrix) != FcResultMatch) {
			return null;
		}
		return matrix;
	}

	SkPangoFont::SkPangoFont(PangoFont *font, SkPangoFontCache &cache) {
		assert(PANGO_IS_FC_FONT(font), L"Your system does not seem to use FontConfig with Pango.");

		hb_font_t *hbFont = pango_font_get_hb_font(font);

		PangoFcFont *fcFont = PANGO_FC_FONT(font);
		FcPattern *pattern = pango_fc_font_get_pattern(fcFont);

		this->transform = fcGetMatrix(pattern, FC_MATRIX);

		PangoFontDescription *description = pango_font_describe_with_absolute_size(font);
		float fontSize = fromPango(pango_font_description_get_size(description));
		// More...
		pango_font_description_free(description);

		skia = SkFont(cache.get(hb_font_get_face(hbFont)).skia, fontSize);
	}

	SkPangoFont::~SkPangoFont() {}

	/**
	 * Cache.
	 */

	SkPangoFontCache::SkPangoFontCache() {}

	SkPangoFontCache::~SkPangoFontCache() {
		for (FontMap::iterator i = fonts.begin(); i != fonts.end(); ++i) {
			delete i->second;
			g_object_unref(i->first);
		}

		for (TypefaceMap::iterator i = typefaces.begin(); i != typefaces.end(); ++i) {
			delete i->second;
			hb_blob_destroy(i->first);
		}
	}

	SkPangoFont &SkPangoFontCache::get(PangoFont *font) {
		FontMap::iterator found = fonts.find(font);
		if (found != fonts.end())
			return *found->second;

		SkPangoFont *f = new SkPangoFont(font, *this);
		fonts[font] = f;
		return *f;
	}

	SkPangoTypeface &SkPangoFontCache::get(hb_face_t *face) {
		hb_blob_t *blob = hb_face_reference_blob(face);
		TypefaceMap::iterator found = typefaces.find(blob);
		if (found != typefaces.end()) {
			hb_blob_destroy(blob);
			return *found->second;
		}

		SkPangoTypeface *f = new SkPangoTypeface(blob);
		typefaces[blob] = f;
		return *f;
	}

}

#endif
