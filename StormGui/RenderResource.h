#pragma once

namespace stormgui {
	class Painter;

	/**
	 * Resource used in DX somewhere.
	 */
	class RenderResource : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Create.
		RenderResource();

		// Destroy.
		~RenderResource();

		// Destroy the resource, lazily created again later.
		void destroy();

		// Forget our owner.
		void forgetOwner();

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

		// Create the resource.
		virtual void create(Painter *owner, ID2D1Resource **out);

	private:
		// The resource itself.
		ID2D1Resource *resource;

		// Our owner (if any).
		Painter *owner;
	};

	/**
	 * Brush (abstract).
	 */
	class Brush : public RenderResource {
		STORM_CLASS;
	public:

		// Get a brush.
		inline ID2D1Brush *brush(Painter *owner) { return get<ID2D1Brush>(owner); }
	};

	/**
	 * Solid brush.
	 */
	class SolidBrush : public Brush {
		STORM_CLASS;
	public:
		STORM_CTOR SolidBrush(Color color);

		virtual void create(Painter *owner, ID2D1Resource **out);

	private:
		Color color;
	};

}


