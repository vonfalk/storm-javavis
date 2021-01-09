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

	static double fcGetDouble(FcPattern *pattern, const char *object, double def) {
		double value = 0;
		if (FcPatternGetDouble(pattern, object, 0, &value) == FcResultMatch)
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
	 * Custom typeface implementation.
	 */

	sk_sp<SkPangoTypeface> SkPangoTypeface::load(const SkTypefaceKey &key) {
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
		return sk_sp<SkPangoTypeface>(new SkPangoTypeface(std::move(fData), std::move(name), style, isFixedWidth, key));
	}

	SkPangoTypeface::SkPangoTypeface(std::unique_ptr<SkFontData> data, SkString family,
									const SkFontStyle &style, bool fixedWidth,
									const SkTypefaceKey &key)
		: SkTypeface_FreeType(style, fixedWidth),
		  key(key),
		  users(0),
		  family(std::move(family)),
		  data(std::move(data)) {}


	void SkPangoTypeface::onGetFamilyName(SkString *familyName) const {
		*familyName = family;
	}

	void SkPangoTypeface::onGetFontDescriptor(SkFontDescriptor *desc, bool *serialize) const {
		*serialize = true;
	}

	std::unique_ptr<SkStreamAsset> SkPangoTypeface::onOpenStream(int *ttcIndex) const {
		*ttcIndex = data->getIndex();
		return data->getStream()->duplicate();
	}

	std::unique_ptr<SkFontData> SkPangoTypeface::onMakeFontData() const {
		return std::unique_ptr<SkFontData>(new SkFontData(*data));
	}

	sk_sp<SkTypeface> SkPangoTypeface::onMakeClone(const SkFontArguments &args) const {
		std::unique_ptr<SkFontData> data = this->cloneFontData(args);
		if (!data)
			return null;
		return sk_sp<SkTypeface>(new SkPangoTypeface(std::move(data), family, fontStyle(), isFixedPitch(), key));
	}

	void SkPangoTypeface::onFilterRec(SkScalerContextRec *rec) const {
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

	/**
	 * Font.
	 */

	SkPangoFont::SkPangoFont(SkPangoFontCache *cache, PangoFont *font) : cache(cache), pango(font) {
		assert(PANGO_IS_FC_FONT(font), L"Your system does not seem to use FontConfig with Pango.");

		PangoFcFont *fcFont = PANGO_FC_FONT(font);
		FcPattern *pattern = fcFont->font_pattern;

		hb_font_t *hbFont = pango_font_get_hb_font(font);
		hb_face_t *hbFace = hb_font_get_face(hbFont);

		SkTypefaceKey key(hb_face_reference_blob(hbFace), pattern);

		double fontSize = fromPango(pango_font_description_get_size(fcFont->description));
		fontSize = fcGetDouble(pattern, FC_PIXEL_SIZE, fontSize);

		this->typeface = cache->typeface(key);
		this->font = SkFont(typeface, fontSize);

		if (SkPangoTypeface *typeface = this->typeface.get())
			atomicIncrement(typeface->users);

		g_object_ref(pango);
	}

	SkPangoFont::~SkPangoFont() {
		// Tell the font cache that we are dying.
		if (cache)
			cache->remove(pango);

		// Remove the ref.
		g_object_unref(pango);

		// Remove the ref from the typeface.
		if (SkPangoTypeface *typeface = this->typeface.get()) {
			if (atomicDecrement(typeface->users) == 1) {
				if (cache)
					cache->remove(typeface);
			}
		}
	}

	/**
	 * Cache.
	 */

	SkPangoFontCache::SkPangoFontCache() {}

	SkPangoFontCache::~SkPangoFontCache() {
		// Make sure the types are destroyed in the proper order.
		for (FontMap::iterator i = fonts.begin(); i != fonts.end(); ++i) {
			i->second->cache = null;
		}
	}

	sk_sp<SkPangoFont> SkPangoFontCache::font(PangoFont *font) {
		os::Lock::L z(lock);

		FontMap::iterator found = fonts.find(font);
		if (found != fonts.end())
			return sk_sp<SkPangoFont>(SkSafeRef(found->second));

		SkPangoFont *created = new SkPangoFont(this, font);
		if (created->typeface.get()) {
			// Save it only if we managed to actually load the font.
			fonts[font] = created;
		}

		return sk_sp<SkPangoFont>(created);
	}

	void SkPangoFontCache::remove(PangoFont *font) {
		os::Lock::L z(lock);

		fonts.erase(font);
	}

	sk_sp<SkPangoTypeface> SkPangoFontCache::typeface(const SkTypefaceKey &key) {
		// We're called from inside a ::font, so we don't need a lock here.

		TypefaceMap::iterator found = typefaces.find(key);
		if (found != typefaces.end())
			return found->second;

		sk_sp<SkPangoTypeface> f = SkPangoTypeface::load(key);
		if (f)
			typefaces[key] = f;
		return f;
	}

	void SkPangoFontCache::remove(SkPangoTypeface *typeface) {
		os::Lock::L z(lock);

		TypefaceMap::iterator found = typefaces.find(typeface->key);

		// Note: We are checking here to ensure that we remove the proper typeface from the
		// cache. Otherwise, we might remove the wrong one in rare cases. (I think it won't happen,
		// but this doesn't hurt).
		if (found != typefaces.end() && found->second.get() == typeface) {
			typefaces.erase(found);
		}
	}

}

#endif
