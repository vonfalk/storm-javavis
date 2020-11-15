#pragma once
#include "Resource.h"
#include "Core/Graphics/Image.h"

namespace gui {

	class Bitmap : public Resource {
		STORM_CLASS;
	public:
		STORM_CTOR Bitmap(Image *src);

#ifdef GUI_GTK
		// Create the bitmap.
		virtual OsResource *create(Painter *owner);
#endif

		// Source image.
		inline Image *STORM_FN image() { return src; }

		// Size.
		Size STORM_FN size();

	protected:
		// Create and update.
		virtual void create(GraphicsMgrRaw *g, void *&result, Cleanup &clean);
		virtual void update(GraphicsMgrRaw *g, void *resource);

	private:
		// Contents.
		Image *src;
	};

}
