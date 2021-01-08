#pragma once

#include "Skia.h"

#ifdef GUI_GTK

#include <fontconfig/fontconfig.h>

namespace gui {

	class SkPangoFontCache;

	/**
	 * Typeface properties. A key in the cache.
	 */
	class SkTypefaceKey {
	public:
		// Create, extract information from Fontconfig. Takes ownership of "blob".
		SkTypefaceKey(hb_blob_t *blob, FcPattern *pattern = null);

		// Copy.
		SkTypefaceKey(const SkTypefaceKey &o);

		// Assign.
		SkTypefaceKey &operator =(const SkTypefaceKey &o);

		// Destroy.
		~SkTypefaceKey();

		// Blob (we have ownership of it).
		hb_blob_t *blob;

		// Should we fake bold.
		bool embolden;

		// Should we apply a transform?
		FcMatrix transform;

		// Compare for equality.
		bool operator ==(const SkTypefaceKey &key) const {
			return blob == key.blob
				&& embolden == key.embolden
				&& transform.xx == key.transform.xx
				&& transform.xy == key.transform.xy
				&& transform.yx == key.transform.yx
				&& transform.yy == key.transform.yy;
		}
	};

}

namespace std {
	template <>
	struct hash<gui::SkTypefaceKey> {
		size_t operator ()(const gui::SkTypefaceKey &key) const {
			return hash<hb_blob_t *>()(key.blob)
				^ hash<bool>()(key.embolden)
				^ hash<float>()(key.transform.xx)
				^ hash<float>()(key.transform.xy)
				^ hash<float>()(key.transform.yx)
				^ hash<float>()(key.transform.yy);
		}
	};
}

namespace gui {

	/**
	 * A typeface (i.e. font file, essentially).
	 */
	class SkPangoTypeface {
	public:
		// Create from a harfbuzz blob containing the typeface.
		SkPangoTypeface(const SkTypefaceKey &key);

		// Destroy.
		~SkPangoTypeface();

		// The Skia typeface. Backed by the storage in the hb_blob_t.
		sk_sp<SkTypeface> skia;

	private:
		// Data extracted from the harfbuzz blob. We probably don't need to keep this alive as
		// SkTypeface probably does that. But it is good to be safe here, as we ask Skia not to copy
		// it.
		sk_sp<SkData> data;
	};

	/**
	 * Font cache.
	 *
	 * TODO: When do we empty the cache? From the code in Pango, it seems like they never remove
	 * anything from their cache, so we might be fine...
	 */
	class SkPangoFontCache {
	public:
		// Create.
		SkPangoFontCache();

		// Destroy.
		~SkPangoFontCache();

		// Get a font.
		SkFont get(PangoFont *font);

		// Get a typeface.
		sk_sp<SkTypeface> get(const SkTypefaceKey &key);

	private:
		// Lock.
		os::Lock lock;

		// Keep track of all the Pango fonts.
		typedef std::unordered_map<PangoFont *, SkFont> FontMap;
		FontMap fonts;

		// Get typefaces.
		typedef std::unordered_map<SkTypefaceKey, sk_sp<SkTypeface>> TypefaceMap;
		TypefaceMap typefaces;
	};

}

#endif
