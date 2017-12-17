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
#ifdef GUI_GTK

		// Get the texture.
		int texture(Painter *owner);

		// Create the texture. Return -1 on failure.
		virtual int create(Painter *owner);

	private:
		// We're storing an integer texture id inside the render resource. Returns -1 if empty.
		int texture() const {
			return ((int)(size_t)resource) - 1;
		}
		void texture(int texture) {
			resource = (OsResource *)(size_t)(texture + 1);
		}

#endif

	private:
		// The resource itself.
		UNKNOWN(PTR_NOGC) OsResource *resource;

		// Our owner (if any).
		Painter *owner;
	};

}

#include "Painter.h"
