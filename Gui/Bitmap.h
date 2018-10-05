#pragma once
#include "RenderResource.h"
#include "Core/Graphics/Image.h"

namespace gui {

	class Bitmap : public RenderResource {
		STORM_CLASS;
	public:
		STORM_CTOR Bitmap(Image *src);

#ifdef GUI_WIN32
		// Helper.
		inline ID2D1Bitmap *bitmap(Painter *owner) { return get<ID2D1Bitmap>(owner); }

		// Create the bitmap.
		virtual void create(Painter *owner, ID2D1Resource **out);
#endif
#ifdef GUI_GTK
		// Create the bitmap.
		virtual OsResource *create(Painter *owner);
#endif

		// Source image.
		inline Image *STORM_FN image() { return src; }

		// Size.
		Size STORM_FN size();

	private:
		// Contents.
		Image *src;
	};

}
