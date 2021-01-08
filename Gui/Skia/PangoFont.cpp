#include "stdafx.h"
#include "PangoFont.h"

#ifdef GUI_GTK

#include <pango/pangofc-font.h>

namespace gui {

	/**
	 * Synthetization parameters from fontconfig.
	 */
	struct FcParams {
		FcMatrix matrix;
		bool embolden;
	};

	static const FcMatrix *fcGetMatrix(FcPattern *pattern, const char *object) {
		FcMatrix *matrix = null;
		if (FcPatternGetMatrix(pattern, object, 0, &matrix) == FcResultMatch)
			return matrix;
		return null;
	}

	static bool fcGetBool(FcPattern *pattern, const char *object, bool def) {
		FcBool value = false;
		if (FcPatternGetBool(pattern, object, 0, &value) == FcResultMatch)
			return value;
		return def;
	}

	static FcParams fcParams(FcPattern *pattern) {
		FcParams p = {
			{ 1, 0, 0, 1 },
			false
		};
		if (const FcMatrix *m = fcGetMatrix(pattern, FC_MATRIX)) {
			p.matrix = *m;
		}

		p.embolden = fcGetBool(pattern, FC_EMBOLDEN, false);

		return p;
	}

	/**
	 * Skia typeface that respects transform matrices from fontconfig and the embolden property.
	 *
	 * More or less a clone of the SkTypeface_stream in SkFontMgr_fontconfig.cpp.
	 */
	class SkTypefaceFc : public SkTypeface_FreeType {
	public:
		SkTypefaceFc(std::unique_ptr<SkFontData> data, SkString family,
					const SkFontStyle &style, bool fixedWidth, FcParams params)
			: SkTypeface_FreeType(style, fixedWidth),
			  family(std::move(family)),
			  data(std::move(data)),
			  params(params) {}

		void onGetFamilyName(SkString *familyName) const {
			*familyName = family;
		}

		void onGetFontDescriptor(SkFontDescriptor *desc, bool *serialize) const {
			*serialize = true;
		}

		std::unique_ptr<SkStreamAsset> onOpenStream(int *ttcIndex) const {
			*ttcIndex = data->getIndex();
			return data->getStream()->duplicate();
		}

		std::unique_ptr<SkFontData> onMakeFontData() const {
			return std::unique_ptr<SkFontData>(new SkFontData(*data));
		}

		sk_sp<SkTypeface> onMakeClone(const SkFontArguments &args) const {
			std::unique_ptr<SkFontData> data = this->cloneFontData(args);
			if (!data)
				return null;
			return sk_make_sp<SkTypefaceFc>(std::move(data), family, fontStyle(), isFixedPitch(), params);
		}

		void onFilterRec(SkScalerContextRec *rec) const {
			SkMatrix m;
			m.setAll(params.matrix.xx, -params.matrix.xy, 0,
					-params.matrix.yx, params.matrix.yy, 0,
					0, 0, 1);

			SkMatrix base;
			rec->getMatrixFrom2x2(&base);

			base.preConcat(m);
			rec->fPost2x2[0][0] = base.getScaleX();
			rec->fPost2x2[0][1] = base.getSkewX();
			rec->fPost2x2[1][0] = base.getSkewY();
			rec->fPost2x2[1][1] = base.getScaleY();

			if (params.embolden)
				rec->fFlags |= SkScalerContext::kEmbolden_Flag;

			SkTypeface_FreeType::onFilterRec(rec);
		}

	private:
		SkString family;
		const std::unique_ptr<const SkFontData> data;
		FcParams params;
	};

	// Make a Fontconfig-based typeface.
	static sk_sp<SkTypeface> makeFcTypeface(sk_sp<SkData> data, FcParams params) {
		std::unique_ptr<SkMemoryStream> stream(new SkMemoryStream(std::move(data)));

		const SkTypeface_FreeType::Scanner scanner;
		SkString name;
		SkFontStyle style;
		bool isFixedWidth = false;
		if (!scanner.scanFont(stream.get(), 0, &name, &style, &isFixedWidth, nullptr))
			return nullptr;

		std::unique_ptr<SkFontData> fData(new SkFontData(std::move(stream), 0, null, 0));
		return sk_sp<SkTypeface>(new SkTypefaceFc(std::move(fData), std::move(name), style, isFixedWidth, params));
	}

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

	SkPangoFont::SkPangoFont(PangoFont *font, SkPangoFontCache &cache) {
		assert(PANGO_IS_FC_FONT(font), L"Your system does not seem to use FontConfig with Pango.");

		hb_font_t *hbFont = pango_font_get_hb_font(font);

		PangoFcFont *fcFont = PANGO_FC_FONT(font);
		FcPattern *pattern = pango_fc_font_get_pattern(fcFont);

		this->transform = null;
		// this->transform = fcGetMatrix(pattern, FC_MATRIX);
		// if (this->transform) {
		// 	PVAR(transform->yy);
		// 	PVAR(hbFont);
		// 	PVAR(hb_font_get_face(hbFont));
		// }

		PangoFontDescription *description = pango_font_describe_with_absolute_size(font);
		float fontSize = fromPango(pango_font_description_get_size(description));
		PVAR(pango_font_description_get_family(description));
		PVAR(hb_font_get_face(hbFont));
		// More...
		pango_font_description_free(description);

		hb_face_t *face = hb_font_get_face(hbFont);
		hb_blob_t *blob = hb_face_reference_blob(face);

		unsigned int size;
		const char *data = hb_blob_get_data(blob, &size);
		auto skData = SkData::MakeWithCopy(data, size);

		skia = SkFont(makeFcTypeface(skData, fcParams(pattern)), fontSize);

		// skia = SkFont(cache.get(hb_font_get_face(hbFont)).skia, fontSize);
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
