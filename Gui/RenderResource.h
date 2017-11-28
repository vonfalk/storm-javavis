#pragma once
#include "Resource.h"

namespace gui {
	class Painter;

	/**
	 * Resource used in DX somewhere, associated with a specific painter.
	 */
	class RenderResource : public Resource {
		STORM_CLASS;
	public:
		// Create.
		RenderResource();

		// Destroy.
		~RenderResource();

		// Destroy the resource, lazily created again later.
		virtual void destroy();

		// Forget our owner.
		void forgetOwner();

#ifdef GUI_WIN32

		// Get the resource, lazily creates it if needed.
		template <class As>
		As *get(Painter *owner) {
			this->owner = owner;
			if (!resource) {
				create(owner, &resource);
				if (resource) {
					owner->addResource(this);
				}
			}
			return static_cast<As*>(resource);
		}

		template <class As>
		As *peek() {
			return static_cast<As*>(resource);
		}

		// Create the resource.
		virtual void create(Painter *owner, ID2D1Resource **out);

#endif

	private:
		// The resource itself.
		ID2D1Resource *resource;

		// Our owner (if any).
		Painter *owner;
	};

}

#include "Painter.h"
