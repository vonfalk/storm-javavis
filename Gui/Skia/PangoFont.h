#pragma once

#include "Skia.h"

#ifdef GUI_GTK

namespace gui {

	class SkPangoFontCache;

	/**
	 * A typeface (i.e. font file, essentially).
	 */
	class SkPangoTypeface {
	public:
		// Create from a harfbuzz blob containing the typeface.
		SkPangoTypeface(hb_blob_t *face);

		// Destroy.
		~SkPangoTypeface();

		// The Skia typeface. Backed by the storage in the hb_blob_t.
		sk_sp<SkTypeface> skia;

	private:
		// Data extracted from the harfbuzz blob. We probably don't need to keep this alive as
		// SkTypeface probably does that. But it is good to be safe here, as we ask Skia not to copy
		// it.
		sk_sp<SkData> data;

		// The harfbuzz blob containing the actual data. We keep this alive as we ask 'data' not to
		// make a copy.
		hb_blob_t *blob;
	};

	/**
	 * A font from Pango that we have extracted a Skia font from.
	 */
	class SkPangoFont {
	public:
		// Create.
		SkPangoFont(PangoFont *font, SkPangoFontCache &cache);

		// Destroy.
		~SkPangoFont();

		// The Skia font.
		SkFont skia;

		// Transform to apply.
		// transform
	};

	/**
	 * Font cache.
	 *
	 * TODO: When do we empty the cache?
	 */
	class SkPangoFontCache {
	public:
		// Create.
		SkPangoFontCache();

		// Destroy.
		~SkPangoFontCache();

		// Get a font.
		SkPangoFont &get(PangoFont *font);

		// Get a typeface.
		SkPangoTypeface &get(hb_face_t *typeface);

	private:
		// Lock.
		os::Lock lock;

		// Keep track of all the Pango fonts.
		typedef std::unordered_map<PangoFont *, SkPangoFont *> FontMap;
		FontMap fonts;

		// Get typefaces.
		typedef std::unordered_map<hb_blob_t *, SkPangoTypeface *> TypefaceMap;
		TypefaceMap typefaces;
	};

}

#endif
