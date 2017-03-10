#pragma once
#include "RenderResource.h"
#include "Core/Graphics/Image.h"

namespace gui {

	class Bitmap : public RenderResource {
		STORM_CLASS;
	public:
		STORM_CTOR Bitmap(Image *src);

		// Helper.
		inline ID2D1Bitmap *bitmap(Painter *owner) { return get<ID2D1Bitmap>(owner); }

		// Create the bitmap.
		virtual void create(Painter *owner, ID2D1Resource **out);

		// Size.
		Size STORM_FN size();

	private:
		// Contents.
		Image *src;
	};

}
