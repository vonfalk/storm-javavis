#include "stdafx.h"
#include "PangoFont.h"

#ifdef GUI_GTK

#include <pango/pangofc-font.h>

namespace gui {

	/**
	 * Font parameters.
	 */

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

	SkTypefaceKey::SkTypefaceKey(hb_blob_t *blob, FcPattern *pattern)
		: blob(blob), embolden(false), transform({1, 0, 0, 1}) {
		hb_blob_reference(blob);
		if (pattern) {
			if (const FcMatrix *m = fcGetMatrix(pattern, FC_MATRIX))
				transform = *m;
			embolden = fcGetBool(pattern, FC_EMBOLDEN, false);
		}
	}

	SkTypefaceKey::SkTypefaceKey(const SkTypefaceKey &o)
		: blob(o.blob), embolden(o.embolden), transform(o.transform) {
		hb_blob_reference(blob);
	}

	SkTypefaceKey &SkTypefaceKey::operator =(const SkTypefaceKey &o) {
		hb_blob_reference(o.blob);
		hb_blob_destroy(blob);
		blob = o.blob;
		embolden = o.embolden;
		transform = o.transform;

		return *this;
	}

	SkTypefaceKey::~SkTypefaceKey() {
		hb_blob_destroy(blob);
	}


	/**
	 * Skia typeface that respects transform matrices from fontconfig and the embolden property.
	 *
	 * More or less a clone of the SkTypeface_stream in SkFontMgr_fontconfig.cpp.
	 */
	class SkTypefaceFc : public SkTypeface_FreeType {
	public:
		SkTypefaceFc(std::unique_ptr<SkFontData> data, SkString family,
					const SkFontStyle &style, bool fixedWidth, const SkTypefaceKey &key)
			: SkTypeface_FreeType(style, fixedWidth),
			  family(std::move(family)),
			  data(std::move(data)),
			  key(key) {}

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
			return sk_make_sp<SkTypefaceFc>(std::move(data), family, fontStyle(), isFixedPitch(), key);
		}

		void onFilterRec(SkScalerContextRec *rec) const {
			SkMatrix m;
			m.setAll(key.transform.xx, -key.transform.xy, 0,
					-key.transform.yx, key.transform.yy, 0,
					0, 0, 1);

			SkMatrix base;
			rec->getMatrixFrom2x2(&base);

			base.preConcat(m);
			rec->fPost2x2[0][0] = base.getScaleX();
			rec->fPost2x2[0][1] = base.getSkewX();
			rec->fPost2x2[1][0] = base.getSkewY();
			rec->fPost2x2[1][1] = base.getScaleY();

			if (key.embolden)
				rec->fFlags |= SkScalerContext::kEmbolden_Flag;

			SkTypeface_FreeType::onFilterRec(rec);
		}

	private:
		SkString family;
		const std::unique_ptr<const SkFontData> data;
		SkTypefaceKey key;
	};

	static sk_sp<SkTypeface> loadTypeface(const SkTypefaceKey &key) {
		unsigned int size;
		const char *data = hb_blob_get_data(key.blob, &size);
		std::unique_ptr<SkMemoryStream> stream(new SkMemoryStream(SkData::MakeWithoutCopy(data, size)));

		const SkTypeface_FreeType::Scanner scanner;
		SkString name;
		SkFontStyle style;
		bool isFixedWidth = false;
		if (!scanner.scanFont(stream.get(), 0, &name, &style, &isFixedWidth, nullptr))
			return nullptr;

		std::unique_ptr<SkFontData> fData(new SkFontData(std::move(stream), 0, null, 0));
		return sk_sp<SkTypeface>(new SkTypefaceFc(std::move(fData), std::move(name), style, isFixedWidth, key));
	}


	/**
	 * Cache.
	 */

	SkPangoFontCache::SkPangoFontCache() {}

	SkPangoFontCache::~SkPangoFontCache() {
		for (FontMap::iterator i = fonts.begin(); i != fonts.end(); ++i) {
			g_object_unref(i->first);
		}
	}

	SkFont SkPangoFontCache::get(PangoFont *font) {
		os::Lock::L z(lock);

		FontMap::iterator found = fonts.find(font);
		if (found != fonts.end())
			return found->second;

		assert(PANGO_IS_FC_FONT(font), L"Your system does not seem to use FontConfig with Pango.");

		hb_font_t *hbFont = pango_font_get_hb_font(font);
		hb_face_t *hbFace = hb_font_get_face(hbFont);

		PangoFcFont *fcFont = PANGO_FC_FONT(font);
		FcPattern *pattern = pango_fc_font_get_pattern(fcFont);

		SkTypefaceKey key(hb_face_reference_blob(hbFace), pattern);

		PangoFontDescription *description = pango_font_describe_with_absolute_size(font);
		float fontSize = fromPango(pango_font_description_get_size(description));
		// More...?
		pango_font_description_free(description);

		sk_sp<SkTypeface> typeface = get(key);
		SkFont f(typeface, fontSize);

		if (typeface) {
			g_object_ref(font);
			fonts[font] = f;
		}
		return f;
	}

	sk_sp<SkTypeface> SkPangoFontCache::get(const SkTypefaceKey &key) {
		TypefaceMap::iterator found = typefaces.find(key);
		if (found != typefaces.end())
			return found->second;

		sk_sp<SkTypeface> f = loadTypeface(key);
		if (f)
			typefaces[key] = f;
		return f;
	}

}

#endif
