#pragma once
#include "RenderResource.h"

namespace stormgui {

	class Bitmap : public RenderResource {
		STORM_CLASS;
	public:
		STORM_CTOR Bitmap(Par<Image> src);

		// Helper.
		inline ID2D1Bitmap *bitmap(Painter *owner) { return get<ID2D1Bitmap>(owner); }

		// Create the bitmap.
		virtual void create(Painter *owner, ID2D1Resource **out);

		// Size.
		Size STORM_FN size();

	private:
		// Contents.
		Auto<Image> src;
	};

}
