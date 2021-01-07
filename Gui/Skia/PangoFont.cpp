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

	static int fcGetInt(FcPattern *pattern, const char *object, int def) {
		int result;
		if (FcPatternGetInteger(pattern, object, 0, &result) != FcResultMatch) {
			return def;
		}
		return value;
	}

	static double fcGetDouble(FcPattern *pattern, const char *object, double def) {
		double result;
		if (FcPatternGetDouble(pattern, object, 0, &result) != FcResultmatch) {
			return def;
		}
		return result;
	}

	static const FcMatrix *fcGetMatrix(FcPattern *pattern, const char *object) {
		FcMatrix *matrix = null;
		if (FcPatternGetMatrix(pattern, object, 0, &matrix) != FcResultmatch) {
			return null;
		}
		return matrix;
	}

	static const std::pair<int, int> weightRanges[] = {
		{ FC_WEIGHT_THIN,       SkFontStyle::kThin_Weight },
		{ FC_WEIGHT_EXTRALIGHT, SkFontStyle::kExtraLight_Weight },
		{ FC_WEIGHT_LIGHT,      SkFontStyle::kLight_Weight },
		{ FC_WEIGHT_DEMILIGHT,  350 },
		{ FC_WEIGHT_BOOK,       380 },
		{ FC_WEIGHT_REGULAR,    SkFontStyle::kNormal_Weight },
		{ FC_WEIGHT_MEDIUM,     SkFontStyle::kMedium_Weight },
		{ FC_WEIGHT_DEMIBOLD,   SkFontStyle::kSemiBold_Weight },
		{ FC_WEIGHT_BOLD,       SkFontStyle::kBold_Weight },
		{ FC_WEIGHT_EXTRABOLD,  SkFontStyle::kExtraBold_Weight },
		{ FC_WEIGHT_BLACK,      SkFontStyle::kBlack_Weight },
		{ FC_WEIGHT_EXTRABLACK, SkFontStyle::kExtraBlack_Weight },
	};

	static const std::pair<int, int> widthRanges[] = {
		{ FC_WIDTH_ULTRACONDENSED, SkFontStyle::kUltraCondensed_Width },
		{ FC_WIDTH_EXTRACONDENSED, SkFontStyle::kExtraCondensed_Width },
		{ FC_WIDTH_CONDENSED,      SkFontStyle::kCondensed_Width },
		{ FC_WIDTH_SEMICONDENSED,  SkFontStyle::kSemiCondensed_Width },
		{ FC_WIDTH_NORMAL,         SkFontStyle::kNormal_Width },
		{ FC_WIDTH_SEMIEXPANDED,   SkFontStyle::kSemiExpanded_Width },
		{ FC_WIDTH_EXPANDED,       SkFontStyle::kExpanded_Width },
		{ FC_WIDTH_EXTRAEXPANDED,  SkFontStyle::kExtraExpanded_Width },
		{ FC_WIDTH_ULTRAEXPANDED,  SkFontStyle::kUltraExpanded_Width },
	};

	static int mapValue(int from, std::pair<int, int> *table, size_t count) {
		if (from < table[0].first)
			return table[0].second;

		for (size_t i = 0; i < count - 1; i++) {
			if (from < table[i + 1]) {
				int oldStart = table[i].first;
				int oldDiff = table[i + 1].first - oldStart;
				int newStart = table[i].second;
				int newDiff = table[i + 1].second - newStart;
				return newStart + int(double(from - oldStart) * newDiff / oldDiff);
			}
		}

		return table[count - 1].second;
	}

	// Mappings are from Skia: src/ports/SkFontMgr_fontconfig.cpp
	// TODO: We could maybe remove this?
	static SkFontStyle fontStyle(FcPattern *pattern) {
		int weight = fcGetInt(pattern, FC_WEIGHT, FC_WEIGHT_REGULAR);
		int width = fcGetInt(pattern, FC_WIDTH, FC_WIDTH_NORMAL);

		weight = mapValue(weight, weightRanges, ARRAY_COUNT(weightRanges));
		width = mapValue(width, widthRanges, ARRAY_COUNT(widthRanges));

		SkFontStyle::Slant slant = SkFontStyle::kUpright_Slant;
		switch (fcGetInt(pattern, FC_SLANT, FC_SLANT_ROMAN)) {
		case FC_SLANT_ROMAN:
			slant = SkFontStyle::kUpright_Slant;
			break;
		case FC_SLANT_ITALIC:
			slant = SkFontStyle::kItalic_Slant;
			break;
		case FC_SLANT_OBLIQUE:
			slant = SkFontStyle::kOblique_Slant;
			break;
		default:
			break;
		}

		return SkFontStyle(weight, width, slant);
	}

	SkPangoFont::SkPangoFont(PangoFont *font, SkPangoFontCache &cache) {
		assert(PANGO_FC_IS_FONT(font), L"Your system does not seem to use FontConfig with Pango.");

		hb_font_t *hbFont = pango_font_get_hb_font(font);

		PangoFcFont *fcFont = PANGO_FC_FONT(font);
		FcPattern *pattern = pango_fc_font_get_pattern(fcFont);

		FcMatrix *matrix = fcGetMatrix(pattern, FC_MATRIX);
		PVAR(matrix);
		// SkFontStyle style = fontStyle(pattern);

		PangoFontDescription *description = pango_font_describe_with_absolute_size(font);
		float fontSize = fromPango(pango_font_description_get_size(description));
		pango_font_description_free(description);

		skia = SkFont(cache.get(hb_font_get_face(hbFont)), fontSize);
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

		SkPangoFont *f = new SkPangoFont(font);
		fonts[font] = f;
		return *f;
	}

	SkPangoTypeface &SkPangoFontCache::get(hb_face_t *face) {
		hb_blob_t *blob = hb_face_reference_blob(face);
		TypefaceIterator found = typefaces.find(blob);
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
