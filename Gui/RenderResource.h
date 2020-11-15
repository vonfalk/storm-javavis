#pragma once
#include "Resource.h"

namespace gui {
	class Painter;

	/**
	 * TODO:
	 *
	 * This file should be removed.
	 */


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
				if (resource)
					attachTo(owner);
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

		// Get the resource, lazily creates it if needed.
		template <class As>
		As *get(Painter *owner) {
			this->owner = owner;
			if (!resource) {
				resource = create(owner);
				destroyFn = &Destroy<As>::destroy;
				if (resource)
					attachTo(owner);
			}
			return (As *)resource;
		}

		// Peek at the resource.
		template <class As>
		As *peek() {
			return (As *)resource;
		}

		// Create the resource.
		virtual OsResource *create(Painter *owner);

#endif

	private:
		// The resource itself.
		OsResource *resource;

		// Function used to destroy 'resource'. Ignored on Windows.
		typedef void (*DestroyFn)(OsResource *res);
		UNKNOWN(PTR_NOGC) DestroyFn destroyFn;

		// Our owner (if any). TODO: Possible to remove?
		Painter *owner;

		// Attach ourselves to our owner.
		void attachTo(Painter *to);
	};

}

#include "Painter.h"
